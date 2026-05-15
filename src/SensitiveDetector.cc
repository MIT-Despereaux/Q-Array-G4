#include "SensitiveDetector.hh"
#include "DataWriter.hh"
#include "TrackInformation.hh"
// #include "ReferenceMessenger.hh"

#include "G4Step.hh"
#include "G4HCofThisEvent.hh"
#include "G4TouchableHistory.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VProcess.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4VSolid.hh"
#include "G4TransportationManager.hh"

// #include <set>

namespace QR
{
  SensitiveDetector::SensitiveDetector(G4String name,
                                       bool bIsReducible,
                                       G4String HCName) : G4VSensitiveDetector(name),
                                                                 mHitCollection(nullptr),
                                                                 mMessenger(nullptr),
                                                                 bIsReducible(bIsReducible),
                                                                 fStepThreshold(0.),
                                                                 bWriteLocalCoords(false),
                                                                 bWriteDeepCopyNo(false)
  {
    collectionName.insert(HCName.empty() ? name : HCName);
    // register with SD manager
    G4SDManager::GetSDMpointer()->AddNewDetector(this);

    mGlobalCopyNumbers.copynum = -1;

    DefineCommands();
  }

  SensitiveDetector::~SensitiveDetector()
  {
    delete mMessenger;
  }

  void SensitiveDetector::Initialize(G4HCofThisEvent *hce)
  {
    mHitCollection = new QRHitsCollection(SensitiveDetectorName,
                                          collectionName[0]);
    G4int hcid = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
    hce->AddHitsCollection(hcid, mHitCollection);
    if (mGlobalCopyNumbers.copynum < 0)
    {
      // Note a sensitive detector can be associated with multiple logical volumes
      // Check that and give an error if its the case
      G4LogicalVolumeStore *logStore = G4LogicalVolumeStore::GetInstance();
      int nLog = 0;
      for (G4LogicalVolume *lvol : *logStore)
      {
        if (lvol->GetSensitiveDetector() == this)
        {
          nLog++;
        }
        if (nLog > 1)
        {
          G4Exception("SensitiveDetector::Initialize", "1",
                      FatalException, "Sensitive detector associated with multiple logical volumes");
        }
      }
      // now find all unique physical volume hierarchies
      double totalmass = 0, totalsurface = 0;
      FindAllPhysicalVolumes(totalmass, totalsurface);
    }
  }

  // This function searches the physical volume tree recursively to find all the volumes
  // associated with this sensitive detector. It also calculates the total mass and surface
  // area of these volumes.
  void SensitiveDetector::SearchVolTree(std::vector<G4VPhysicalVolume *> &searchpath,
                                        double &totalmass, double &totalarea)
  {
    // If the search path is empty, return
    if (searchpath.empty())
      return;

    // Get the logical volume of the current physical volume
    G4LogicalVolume *lvol = searchpath.back()->GetLogicalVolume();

    // If the logical volume is associated with this sensitive detector, update the global
    // copy number and add the mass and surface area to the total
    if (lvol->GetSensitiveDetector() == this)
    {
      // Get the path to the current physical volume in the global copy number tree
      PVPath *node = &mGlobalCopyNumbers;
      for (const auto *pv : searchpath)
      {
        node = &(node->branches[pv]);
      }

      // Update the global copy number and add the mass and surface area to the total
      node->copynum = mGlobalCopyNumbers.copynum++;
      totalmass += lvol->GetMass();
      totalarea += lvol->GetSolid()->GetSurfaceArea();
    }

    // Recursively search the daughters of the current physical volume
    for (size_t i = 0; i < lvol->GetNoDaughters(); ++i)
    {
      searchpath.push_back(lvol->GetDaughter(i));
      SearchVolTree(searchpath, totalmass, totalarea);
    }

    // Remove the current physical volume from the search path
    searchpath.pop_back();
  }

  void SensitiveDetector::FindAllPhysicalVolumes(double &totalmass,
                                                 double &totalarea)
  {
    G4TransportationManager *tm = G4TransportationManager::GetTransportationManager();
    G4VPhysicalVolume *world = *(tm->GetWorldsIterator());
    if (!world)
    {
      G4ExceptionDescription msg;
      msg << "No world volume registered with transportation manager!";
      G4Exception("GPSVolumeSampling::SetNewValue", "1", FatalException, msg);
    }
    totalmass = 0;
    totalarea = 0;
    mGlobalCopyNumbers.copynum = 0;
    std::vector<G4VPhysicalVolume *> searchpath;
    searchpath.push_back(world);
    SearchVolTree(searchpath, totalmass, totalarea);
  }

  G4int SensitiveDetector::GetGlobalCopyNumber(const G4VTouchable *touchable) const
  {
    const PVPath *node = &mGlobalCopyNumbers;
    try
    {
      for (G4int i = touchable->GetHistoryDepth(); i > -1; --i)
      {
        node = &(node->at(touchable->GetVolume(i)));
      }
    }
    catch (std::out_of_range &)
    {
      return -1;
    }
    return node->copynum;
  }

  HitData *FindParent(QRHitsCollection *hc, HitData *hit)
  {
    G4int parentTrackID = hit->parentTrackID;
    G4int trackID = hit->trackID;
    G4int prevStepNum = hit->stepNum - 1;
    G4double t0 = hit->t0;
    G4String parentProcess = hit->creatorProcess;
    for (int i = 0; i < hc->entries(); ++i)
    {
      HitData *h = dynamic_cast<HitData *>(hc->GetHit(i));
      if (h->trackID == parentTrackID && h->t1 == t0 && h->stepProcess == parentProcess)
      {
        return h;
      }
      if (h->stepNum == prevStepNum && h->trackID == trackID)
      {
        return h;
      }
    }
    return nullptr;
  }

  void FillInitialVertex(HitData *hit, G4Step *step)
  {
    G4Track *track = step->GetTrack();
    TrackInformation *trackInfo = static_cast<TrackInformation *>(track->GetUserInformation());
    hit->vx = trackInfo->mOriginalPosition;
    hit->vp = trackInfo->mOriginalMomentum;
    hit->primaryVertexParticle = trackInfo->particleDefinition->GetParticleName();
    // G4cout << "HitData::FillInitialVertex [vx]: " << hit->vx << G4endl;
    // G4cout << "HitData::FillInitialVertex [vp]: " << hit->vp << G4endl;
    return;
  }

  G4bool SensitiveDetector::ProcessHits(G4Step *step, G4TouchableHistory *)
  {
    if (!Accept(step))
      return false;
    G4int copyno = -1;
    if (bWriteDeepCopyNo)
    {
      copyno = GetGlobalCopyNumber(step->GetPreStepPoint()->GetTouchable());
    }
    HitData *hit = new HitData(step->GetTrack(), step,
                               bWriteLocalCoords,
                               copyno);
    // Find the parent of this hit in the HC
    HitData *hitParent = FindParent(mHitCollection, hit);
    hit->decayAncestor = hitParent;
    hit->decayAncestorHashID = hitParent ? hitParent->hash() : 0;

    // Add the initial vertex to the hit
    FillInitialVertex(hit, step);

    mHitCollection->insert(hit);
    return true;
  }

  void SensitiveDetector::EndOfEvent(G4HCofThisEvent *)
  {
  }

  bool SensitiveDetector::Accept(const G4Step *step)
  {
    // default take anything with >0 edep
    G4double edep = step->GetTotalEnergyDeposit() + step->GetNonIonizingEnergyDeposit();
    return step && edep > fStepThreshold;
  }

  void SensitiveDetector::DefineCommands()
  {
    mMessenger = new G4GenericMessenger(this, "/QR/sensDet/", "Generic sensitive detector control");

    using Cmd = G4GenericMessenger::Command;

    G4String name = SensitiveDetectorName;
    Cmd &setReducibleCmd = mMessenger->DeclareProperty(name + "/setReducible", bIsReducible,
                                                       "Set whether this detector is reducible");
    setReducibleCmd.SetParameterName("reducible", true);
    setReducibleCmd.SetDefaultValue("true");

    Cmd &setStepThresholdCmd = mMessenger->DeclareProperty(name + "/setStepThreshold", fStepThreshold,
                                                           "Set the minimum step threshold for hits");
    setStepThresholdCmd.SetParameterName("thres", false); //< Parameter not ommittable
    setStepThresholdCmd.SetRange("thres>=0.");
    setStepThresholdCmd.SetDefaultValue("0.");

    Cmd &setWriteLocalCoordsCmd = mMessenger->DeclareProperty(name + "/setWriteLocalCoords", bWriteLocalCoords,
                                                              "Set whether to write local coordinates");
    setWriteLocalCoordsCmd.SetParameterName("local", true);
    setWriteLocalCoordsCmd.SetDefaultValue("false");

    Cmd &setWriteDeepCopyNoCmd = mMessenger->DeclareProperty(name + "/setWriteDeepCopyNo", bWriteDeepCopyNo,
                                                             "Set whether to write deep copy number");
    setWriteDeepCopyNoCmd.SetParameterName("deepcopy", true);
    setWriteDeepCopyNoCmd.SetDefaultValue("false");
  }
}

// FluxCounter::FluxCounter(const G4String &name, bool killTracks,
//                          const std::set<G4VPhysicalVolume *> &restrvols) : SensitiveDetector(name, 0, 0),
//                                                                            fKillTracks(killTracks),
//                                                                            fRestrictVols(restrvols)
// {
//   mMessenger->AddParameter(fKillTracks, "killtracks",
//                            "Kill tracks crossing the boundary");
// }

// FluxCounter::~FluxCounter()
// {
// }

// bool FluxCounter::Accept(const G4Step *step)
// {
//   bool boundary = (step->GetPreStepPoint()->GetStepStatus() == fGeomBoundary ||
//                    step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary);
//   bool volmatch = true;
//   if (boundary && !fRestrictVols.empty())
//   {
//     volmatch = fRestrictVols.count(step->GetPostStepPoint()->GetPhysicalVolume());
//   }

//   return boundary && volmatch;
// }

// G4bool FluxCounter::ProcessHits(G4Step *step, G4TouchableHistory *history)
// {
//   if (SensitiveDetector::ProcessHits(step, history))
//   {
//     if (fKillTracks)
//       step->GetTrack()->SetTrackStatus(fStopAndKill);
//     return true;
//   }
//   return false;
// }
