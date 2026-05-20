#include "HitData.hh"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4VTouchable.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4VProcess.hh"
#include "G4VAnalysisManager.hh"
#include "G4EventManager.hh"
#include "G4Event.hh"
#include "G4Circle.hh"
#include "G4Polyline.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4VVisManager.hh"

#include <algorithm>
#include <numeric>

namespace QArray
{
  // initialize the allocators
  G4ThreadLocal G4Allocator<HitData> *HitDataAllocator = nullptr;

  bool HitData::bsWriteDecayAncestors = true;
  bool HitData::bsIncludeNonIonizingEdep = true;

  std::unordered_map<std::string, HitData::REDUCELEVEL> const HitData::csRLVLTABLE = {
      {"MINIMAL", HitData::kMiminal},
      {"FULL", HitData::kFull},
  };

  HitData::HitData(const G4Track *track, const G4Step *step,
                   bool localCoords, int copynum) : G4VHit(),
                                                    bWriteLocalCoords(localCoords),
                                                    decayAncestor(0), decayDaughter(0),
                                                    bFilled(false)
  {
    // set some relevant defaults
    eventID = -1;
    trackID = -1;
    stepNum = -1;
    parentTrackID = -1;
    particleCode = -1;
    copyno = copynum;
    nHits = 0;
    ke = -1;
    hashID = -1;
    decayAncestorHashID = -1;

    Fill(track, step);
  }

  HitData::~HitData()
  {
  }

  void HitData::Fill(const G4Track *track, const G4Step *step)
  {
    if (!track)
      return;

    bFilled = true;
    nHits = 1;
    // first fill info from track
    const G4ParticleDefinition *pd = track->GetParticleDefinition();
    const G4VPhysicalVolume *vol = track->GetVolume();
    const G4VProcess *trkProc = track->GetCreatorProcess();

    eventID = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();
    trackID = track->GetTrackID();
    stepNum = track->GetCurrentStepNumber();
    parentTrackID = track->GetParentID();
    particleName = pd->GetParticleName();
    incidentParticle = pd->GetParticleName();
    particleCode = pd->GetPDGEncoding();
    volname = (vol ? vol->GetName() : "");
    G4VPhysicalVolume *nextvol = track->GetNextVolume();
    volnameNext = (nextvol ? nextvol->GetName() : "");
    if (vol && copyno < 0)
      copyno = vol->GetCopyNo();
    if (trkProc)
      creatorProcess = trkProc->GetProcessName();
    else
      creatorProcess = "Primary";

    ke = track->GetKineticEnergy();
    t0 = t1 = track->GetGlobalTime();
    x0 = x1 = track->GetPosition();
    p0 = p1 = track->GetMomentum();
    dx = track->GetStepLength();
    edep = edepER = edepNR = 0;
    stepProcess = "";
    boundary = 0;

    // now try to fill step info
    if (!step)
      step = track->GetStep();
    if (step)
    {
      const G4StepPoint *prepoint = step->GetPreStepPoint();
      const G4StepPoint *postpoint = step->GetPostStepPoint();
      const G4VProcess *stepProc = postpoint->GetProcessDefinedStep();
      edep = step->GetTotalEnergyDeposit();
      if (bsIncludeNonIonizingEdep)
        edep += step->GetNonIonizingEnergyDeposit();
      if (particleCode > 1000) // todo: is this the right cut?
        edepNR = edep;
      else
        edepER = edep;
      dx = step->GetStepLength();
      stepProcess = (stepProc ? stepProc->GetProcessName() : "");

      ke = prepoint->GetKineticEnergy();
      t0 = prepoint->GetGlobalTime();
      x0 = prepoint->GetPosition();
      p0 = prepoint->GetMomentum();

      t1 = postpoint->GetGlobalTime();
      x1 = postpoint->GetPosition();
      p1 = postpoint->GetMomentum();

      // If boundary == 1 or 3 then the particle is "incident"
      boundary = (1 * (prepoint->GetStepStatus() == fGeomBoundary) +
                  2 * (postpoint->GetStepStatus() == fGeomBoundary));

      const G4VTouchable *touchable = prepoint->GetTouchable();
      if (touchable && bWriteLocalCoords)
      {
        G4AffineTransform tform = touchable->GetHistory()->GetTopTransform();
        local_x0 = tform.TransformPoint(x0);
        local_x1 = tform.TransformPoint(x1);
        local_p0 = tform.TransformAxis(p0);
        local_p1 = tform.TransformAxis(p1);
      }
    }

    hashID = hash();

    // copy ancestor data from track info if it exists
    // TODO: Modify this to save the ancestor and daughter data.
    // const BGMTrackInfo *trackinfo =
    //     static_cast<const BGMTrackInfo *>(track->GetUserInformation());
    // if (trackinfo)
    // {
    //   decayAncestor = trackinfo->GetDecayAncestorData();
    //   decayDaughter = trackinfo->GetDecayDaughterData();
    // }
  }

  enum MANIPMODE
  {
    CREATE,
    FILL
  };

  void ManipDColumn(G4VAnalysisManager *AMan, MANIPMODE mode,
                    int ntupleid, int id, const G4String &name, G4double val)
  {
    // G4cout << (mode == CREATE ? "Creating" : "Setting")
    //        << " column " << id << " of ntuple " << ntupleid
    //        << " (name=" << name << ", type=D) to " << val << G4endl;
    if (mode == CREATE)
      AMan->CreateNtupleDColumn(ntupleid, name);
    else
      AMan->FillNtupleDColumn(ntupleid, id, val);
  }

  void ManipIColumn(G4VAnalysisManager *AMan, MANIPMODE mode,
                    int ntupleid, int id, const G4String &name, G4int val)
  {
    // G4cout << (mode == CREATE ? "Creating" : "Setting")
    //        << " column " << id << " of ntuple " << ntupleid
    //        << " (name=" << name << ", type=I) to " << val << G4endl;
    if (mode == CREATE)
      AMan->CreateNtupleIColumn(ntupleid, name);
    else
      AMan->FillNtupleIColumn(ntupleid, id, val);
  }

  void ManipSColumn(G4VAnalysisManager *AMan, MANIPMODE mode,
                    int ntupleid, int id, const G4String &name,
                    const G4String &val)
  {
    // G4cout << (mode == CREATE ? "Creating" : "Setting")
    //        << " column " << id << " of ntuple " << ntupleid
    //        << " (name=" << name << ", type=S) to " << val << G4endl;
    if (mode == CREATE)
      AMan->CreateNtupleSColumn(ntupleid, name);
    else
      AMan->FillNtupleSColumn(ntupleid, id, val);
  }

  void ManipulateNtuple(G4VAnalysisManager *AMan,
                        HitData::REDUCELEVEL reduce,
                        HitData *hit = 0, int ntupleid = 0)
  {
    MANIPMODE mode = (hit && hit->bFilled) ? FILL : CREATE;
    // need to dereference pointers for create mode...
    static HitData dummyhit;
    if (!hit)
      hit = &dummyhit;
    if (!hit->decayAncestor)
      hit->decayAncestor = &dummyhit;
    if (!hit->decayDaughter)
      hit->decayDaughter = &dummyhit;
    int id = 0;

    // Basic info
    ManipIColumn(AMan, mode, ntupleid, id++, "eventID", hit->eventID);
    ManipIColumn(AMan, mode, ntupleid, id++, "trackID", hit->trackID);
    ManipIColumn(AMan, mode, ntupleid, id++, "stepNum", hit->stepNum);
    ManipIColumn(AMan, mode, ntupleid, id++, "parentTrackID", hit->parentTrackID);
    ManipIColumn(AMan, mode, ntupleid, id++, "hashID", hit->hashID);
    ManipIColumn(AMan, mode, ntupleid, id++, "ancestorID", hit->decayAncestorHashID);
    ManipIColumn(AMan, mode, ntupleid, id++, "boundary", hit->boundary);
    ManipDColumn(AMan, mode, ntupleid, id++, "ke", hit->ke);
    ManipDColumn(AMan, mode, ntupleid, id++, "edep", hit->edep);
    ManipDColumn(AMan, mode, ntupleid, id++, "edepER", hit->edepER);
    ManipDColumn(AMan, mode, ntupleid, id++, "edepNR", hit->edepNR);
    ManipDColumn(AMan, mode, ntupleid, id++, "t0", hit->t0);
    ManipDColumn(AMan, mode, ntupleid, id++, "t1", hit->t1);
    ManipIColumn(AMan, mode, ntupleid, id++, "copyno", hit->copyno);
    ManipIColumn(AMan, mode, ntupleid, id++, "nHits", hit->nHits);
    ManipIColumn(AMan, mode, ntupleid, id++, "particleCode",
                 hit->particleCode);
    ManipSColumn(AMan, mode, ntupleid, id++, "particleName",
                 hit->particleName);
    ManipSColumn(AMan, mode, ntupleid, id++, "incidentParticle", hit->incidentParticle);
    ManipSColumn(AMan, mode, ntupleid, id++, "creatorProcess",
                 hit->creatorProcess);
    ManipSColumn(AMan, mode, ntupleid, id++, "stepProcess",
                 hit->stepProcess);
    ManipDColumn(AMan, mode, ntupleid, id++, "vx0", hit->vx[0]);
    ManipDColumn(AMan, mode, ntupleid, id++, "vy0", hit->vx[1]);
    ManipDColumn(AMan, mode, ntupleid, id++, "vz0", hit->vx[2]);
    ManipDColumn(AMan, mode, ntupleid, id++, "vpx0", hit->vp[0]);
    ManipDColumn(AMan, mode, ntupleid, id++, "vpy0", hit->vp[1]);
    ManipDColumn(AMan, mode, ntupleid, id++, "vpz0", hit->vp[2]);
    ManipSColumn(AMan, mode, ntupleid, id++, "primaryVertexParticle",
                 hit->primaryVertexParticle);

    // Full info
    if (reduce >= HitData::kFull)
    {
      if (hit->bWriteLocalCoords)
      {
        ManipDColumn(AMan, mode, ntupleid, id++, "x1", hit->local_x1[0]);
        ManipDColumn(AMan, mode, ntupleid, id++, "y1", hit->local_x1[1]);
        ManipDColumn(AMan, mode, ntupleid, id++, "z1", hit->local_x1[2]);
        ManipDColumn(AMan, mode, ntupleid, id++, "px0", hit->local_p0[0]);
        ManipDColumn(AMan, mode, ntupleid, id++, "py0", hit->local_p0[1]);
        ManipDColumn(AMan, mode, ntupleid, id++, "pz0", hit->local_p0[2]);
        ManipDColumn(AMan, mode, ntupleid, id++, "px1", hit->local_p1[0]);
        ManipDColumn(AMan, mode, ntupleid, id++, "py1", hit->local_p1[1]);
        ManipDColumn(AMan, mode, ntupleid, id++, "pz1", hit->local_p1[2]);
      }
      else
      {
        ManipDColumn(AMan, mode, ntupleid, id++, "x1", hit->x1[0]);
        ManipDColumn(AMan, mode, ntupleid, id++, "y1", hit->x1[1]);
        ManipDColumn(AMan, mode, ntupleid, id++, "z1", hit->x1[2]);
        ManipDColumn(AMan, mode, ntupleid, id++, "px0", hit->p0[0]);
        ManipDColumn(AMan, mode, ntupleid, id++, "py0", hit->p0[1]);
        ManipDColumn(AMan, mode, ntupleid, id++, "pz0", hit->p0[2]);
        ManipDColumn(AMan, mode, ntupleid, id++, "px1", hit->p1[0]);
        ManipDColumn(AMan, mode, ntupleid, id++, "py1", hit->p1[1]);
        ManipDColumn(AMan, mode, ntupleid, id++, "pz1", hit->p1[2]);
      }
      ManipDColumn(AMan, mode, ntupleid, id++, "dx", hit->dx);
      ManipSColumn(AMan, mode, ntupleid, id++, "volname", hit->volname);
      ManipSColumn(AMan, mode, ntupleid, id++, "volnameNext", hit->volnameNext);
    }

    if (HitData::bsWriteDecayAncestors)
    {
      // if (reduce <= HitData::ANCESTOR)
      // {
      //   ManipIColumn(AMan, mode, ntupleid, id++, "decayAncestor.trackID",
      //                hit->decayAncestor->trackID);
      //   ManipIColumn(AMan, mode, ntupleid, id++, "decayAncestor.particleCode",
      //                hit->decayAncestor->particleCode);
      //   ManipSColumn(AMan, mode, ntupleid, id++, "decayAncestor.particleName",
      //                hit->decayAncestor->particleName);
      //   ManipIColumn(AMan, mode, ntupleid, id++, "decayAncestor.parentTrackID",
      //                hit->decayAncestor->parentTrackID);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayAncestor.t1",
      //                hit->decayAncestor->t1);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayAncestor.x1",
      //                hit->decayAncestor->x1[0]);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayAncestor.y1",
      //                hit->decayAncestor->x1[1]);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayAncestor.z1",
      //                hit->decayAncestor->x1[2]);
      //   ManipSColumn(AMan, mode, ntupleid, id++, "decayAncestor.volname",
      //                hit->decayAncestor->volname);
      // }
      // if (reduce <= HitData::DAUGHTER)
      // {
      //   ManipIColumn(AMan, mode, ntupleid, id++, "decayDaughter.trackID",
      //                hit->decayDaughter->trackID);
      //   ManipIColumn(AMan, mode, ntupleid, id++, "decayDaughter.particleCode",
      //                hit->decayDaughter->particleCode);
      //   ManipSColumn(AMan, mode, ntupleid, id++, "decayDaughter.particleName",
      //                hit->decayDaughter->particleName);
      //   ManipIColumn(AMan, mode, ntupleid, id++, "decayDaughter.parentTrackID",
      //                hit->decayDaughter->parentTrackID);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayDaughter.ke",
      //                hit->decayDaughter->ke);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayDaughter.px0",
      //                hit->decayDaughter->p0[0]);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayDaughter.py0",
      //                hit->decayDaughter->p0[1]);
      //   ManipDColumn(AMan, mode, ntupleid, id++, "decayDaughter.pz0",
      //                hit->decayDaughter->p0[2]);
      // }
    }
  }

  // Defines the Ntuple.
  void HitData::DefineNtuple(G4VAnalysisManager *AMan, G4int ntupleid,
                             REDUCELEVEL reduce, bool finish)
  {
    ManipulateNtuple(AMan, reduce, 0, ntupleid);
    if (finish)
      AMan->FinishNtuple(ntupleid);
  }

  // Fills the Ntuple.
  void HitData::FillNtuple(G4VAnalysisManager *AMan, G4int ntupleid,
                           REDUCELEVEL reduce, bool finish)
  {
    if (bFilled)
      ManipulateNtuple(AMan, reduce, this, ntupleid);
    if (finish)
      AMan->AddNtupleRow(ntupleid);
  }

#define TESTSAME(ob, def) \
  if (ob != right.ob)     \
  {                       \
    ob = def;             \
  }

  HitData &HitData::operator+=(const HitData &right)
  {
    // first test if we have been filled
    if (!right.bFilled)
      return *this;
    if (!bFilled)
    {
      *this = right;
      return *this;
    }

    nHits += right.nHits;

    // most things don't make sense when added!
    TESTSAME(eventID, -1);
    TESTSAME(trackID, -1);
    TESTSAME(stepNum, -1);
    TESTSAME(particleCode, -1);
    TESTSAME(particleName, "");
    TESTSAME(parentTrackID, -1);

    // Use the primary particle of the smaller "hit"
    if (*this < right)
    {
      incidentParticle = this->incidentParticle;
    }
    else
    {
      incidentParticle = right.incidentParticle;
    }

    if (edep)
    {
      x0 = (x0 * edep + right.x0 * right.edep) / (edep + right.edep);
      x1 = (x1 * edep + right.x1 * right.edep) / (edep + right.edep);
      p0 = (p0 * edep + right.p0 * right.edep) / (edep + right.edep);
      p1 = (p1 * edep + right.p1 * right.edep) / (edep + right.edep);
    }
    else
    {
      x0 = right.x0;
      x1 = right.x1;
      p0 = right.p0;
      p1 = right.p1;
    }

    boundary = (1 & boundary) + (2 & right.boundary);

    edep += right.edep;
    edepER += right.edepER;
    edepNR += right.edepNR;

    TESTSAME(ke, -1); // TODO: improve this?
    t0 = std::min(t0, right.t0);
    t1 = std::max(t1, right.t1);

    dx += right.dx;

    TESTSAME(volname, "");
    TESTSAME(volnameNext, "");
    TESTSAME(copyno, -1);
    TESTSAME(creatorProcess, "");
    TESTSAME(stepProcess, "");

    TESTSAME(decayAncestor, 0);
    TESTSAME(decayAncestorHashID, -1);
    TESTSAME(decayDaughter, 0);

    TESTSAME(vx, G4ThreeVector(0, 0, 0));
    TESTSAME(vp, G4ThreeVector(0, 0, 0));
    TESTSAME(primaryVertexParticle, "");

    return *this;
  }

  HitData HitData::operator+(const HitData &right) const
  {
    HitData hit(*this);
    return hit += right;
  }

  void HitData::Draw()
  {
    G4VVisManager *vm = G4VVisManager::GetConcreteInstance();
    if (vm)
    {
      G4Circle circle(x1);
      circle.SetScreenDiameter(5);
      circle.SetFillStyle(G4Circle::filled);
      // hit color
      G4Colour color(1, 0, 0); // red: default (usually fluxes)
      if (trackID == 1 && stepNum <= 1)
        color = G4Colour(0, 0, 1); // blue: primaries
      else if (edep > 0)
        color = G4Colour(0, 1, 0); // green: detector hits
      circle.SetVisAttributes(color);
      vm->Draw(circle);

      /*
      // draw a line from the initial to final
      G4Polyline line;
      line.push_back(G4Point3D(x0[0], x0[1], x0[2]));
      line.push_back(G4Point3D(x1[0], x1[1], x1[2]));
      line.SetVisAttributes(color);
      vm->Draw(line);
      */
    }
  }
}