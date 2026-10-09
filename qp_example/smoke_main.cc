#include "QuasiparticleDetectorConstruction.hh"
#include "QuasiparticleConfigManager.hh"
#include "G4CMPConfigManager.hh"
#include "G4CMPPhysics.hh"
#include "G4CMPStackingAction.hh"
#include "G4RunManager.hh"
#include "G4PhysListFactory.hh"
#include "G4GeneralParticleSource.hh"
#include "G4GenericMessenger.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4VUserActionInitialization.hh"
#include "G4UserRunAction.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4UImanager.hh"
#include "G4Event.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4Box.hh"
#include "G4AffineTransform.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4Navigator.hh"
#include "G4Polyhedron.hh"
#include "G4TransportationManager.hh"
#include "G4CMPLogicalBorderSurface.hh"
#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4Version.hh"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <functional>
#include <tuple>
#include <vector>
#include <iomanip>
#include <sstream>
#include <set>
#include <map>
#include <cstdlib>
#include <cmath>
#ifdef QARRAY_COMPRESS_STEPS
#include <zlib.h>
#endif

namespace QArray
{
  class TextOutput
  {
  public:
    explicit TextOutput(const std::string& path)
    {
#ifdef QARRAY_COMPRESS_STEPS
      fPath = path + ".gz";
      fFile = gzopen(fPath.c_str(), "wb6");
      if (fFile) gzbuffer(fFile, 1024 * 1024);
#else
      fPath = path;
      fFile.open(fPath);
#endif
    }
    ~TextOutput()
    {
#ifdef QARRAY_COMPRESS_STEPS
      if (fFile) gzclose(fFile);
#endif
    }
    G4bool Ready() const
    {
#ifdef QARRAY_COMPRESS_STEPS
      return fFile != nullptr;
#else
      return static_cast<G4bool>(fFile);
#endif
    }
    void Write(const std::string& text)
    {
#ifdef QARRAY_COMPRESS_STEPS
      if (fFile) gzwrite(fFile, text.data(), static_cast<unsigned int>(text.size()));
#else
      fFile << text;
#endif
    }
    void Flush()
    {
#ifdef QARRAY_COMPRESS_STEPS
      if (fFile) gzflush(fFile, Z_SYNC_FLUSH);
#else
      fFile.flush();
#endif
    }
    const std::string& Path() const { return fPath; }
  private:
    std::string fPath;
#ifdef QARRAY_COMPRESS_STEPS
    gzFile fFile = nullptr;
#else
    std::ofstream fFile;
#endif
  };

  struct SmokeState
  {
    G4int maxQPSteps = 1000;
    G4bool requireQPs = false;
    G4bool recordAllQPSteps = false;
    G4double qpSampleIntervalNs = 0.;
    G4String expectedMaterial;
    G4String geometryDumpFile;
    G4bool mergeConnectedAl = false;
    nlohmann::json mergeReport;
    G4String expectedGroundPlaneMaterial;
    G4double expectedGroundPlaneThicknessNm = -1.;
    G4bool geometryValid = true;
    G4bool aborted = false;
    G4bool phononReported = false;
    G4bool qpReported = false;
    G4bool movementReported = false;
    G4int completedEvents = 0;
    G4int phononTracks = 0;
    G4int qpTracks = 0;
    G4long qpStepRows = 0;
    G4long qpSampleRows = 0;
    G4long qpStepsTotal = 0;
    G4int limitedQPTracks = 0;
    G4double primaryEdep = 0.;
    nlohmann::json primary;
    nlohmann::json groundPlane;
    std::set<G4int> movingQPTracks;
    std::map<G4int, G4double> nextQPSampleTime;
    TextOutput steps{"steps.csv"};
    TextOutput samples{"qp_samples.csv"};
    SmokeState()
    {
      WriteSteps("event,track,parent,step,particle,pre_energy_eV,post_energy_eV,edep_eV,"
                 "pre_time_ns,time_ns,pre_x_mm,pre_y_mm,pre_z_mm,x_mm,y_mm,z_mm,"
                 "pre_dir_x,pre_dir_y,pre_dir_z,post_dir_x,post_dir_y,post_dir_z,"
                 "pre_material,post_material,pre_volume,post_volume,process,status,weight,"
                 "step_length_mm\n");
      WriteSamples("event,track,parent,particle,record_kind,time_ns,x_mm,y_mm,z_mm,"
                   "energy_eV,dir_x,dir_y,dir_z,material,volume,status,weight,source_step\n");
    }
    G4bool StepsReady() const
    {
      return steps.Ready() && samples.Ready();
    }
    void WriteSteps(const std::string& row) { steps.Write(row); }
    void WriteSamples(const std::string& row) { samples.Write(row); }
    void FlushSteps()
    {
      steps.Flush();
      samples.Flush();
    }
  };

  class SmokeGenerator : public G4VUserPrimaryGeneratorAction
  {
  public:
    explicit SmokeGenerator(SmokeState& state) : fState(state) {}
    void GeneratePrimaries(G4Event* event) override
    {
      fGPS.GeneratePrimaryVertex(event);
      const auto* vertex = event->GetPrimaryVertex();
      const auto* particle = vertex->GetPrimary();
      fState.primary = {{"particle", particle->GetParticleDefinition()->GetParticleName()},
                        {"energy_eV", particle->GetKineticEnergy() / eV},
                        {"position_mm", {vertex->GetX0()/mm, vertex->GetY0()/mm, vertex->GetZ0()/mm}}};
    }
  private:
    SmokeState& fState;
    G4GeneralParticleSource fGPS;
  };

  // Record the ground plane and fail if it, or any volume patterned into it,
  // does not match the macro's expected material and thickness.
  void CheckGeometry(SmokeState& fState)
  {
    const auto* groundPlane =
        G4PhysicalVolumeStore::GetInstance()->GetVolume("GroundPlane", false);
    const auto* solid = groundPlane
        ? dynamic_cast<const G4Box*>(groundPlane->GetLogicalVolume()->GetSolid()) : nullptr;
    const auto material = groundPlane
        ? groundPlane->GetLogicalVolume()->GetMaterial()->GetName() : G4String("missing");
    const auto thicknessNm = solid ? 2. * solid->GetZHalfLength()/nm : -1.;
    // Every volume patterned into the film (resonators, feedline, gaps) must
    // span the full film thickness; a thinner one leaves Al above or below it.
    G4int filmVolumes = 0;
    G4int thinFilmVolumes = 0;
    nlohmann::json thinNames = nlohmann::json::array();
    std::function<void(const G4LogicalVolume*)> checkFilm = [&](const G4LogicalVolume* logical)
    {
      for (std::size_t i = 0; i < logical->GetNoDaughters(); ++i)
      {
        const auto* daughter = logical->GetDaughter(i)->GetLogicalVolume();
        G4ThreeVector low, high;
        daughter->GetSolid()->BoundingLimits(low, high);
        ++filmVolumes;
        // 0.01 nm tolerance: the stock feedline is deliberately ~2 pm thicker.
        if (std::abs((high.z()-low.z())/nm - thicknessNm) > 0.01)
        {
          ++thinFilmVolumes;
          if (thinNames.size() < 10)
            thinNames.push_back({logical->GetDaughter(i)->GetName(), (high.z()-low.z())/nm});
        }
        checkFilm(daughter);
      }
    };
    if (groundPlane) checkFilm(groundPlane->GetLogicalVolume());
    fState.groundPlane = {{"material", material}, {"thickness_nm", thicknessNm},
                          {"film_volumes", filmVolumes},
                          {"film_volumes_not_full_thickness", thinFilmVolumes},
                          {"first_thin_film_volumes_nm", thinNames}};
    G4cout << "SMOKE_GROUND_PLANE " << fState.groundPlane.dump() << G4endl;

    fState.geometryValid = groundPlane && solid;
    if (!fState.expectedGroundPlaneMaterial.empty())
      fState.geometryValid = fState.geometryValid &&
        material == fState.expectedGroundPlaneMaterial;
    if (fState.expectedGroundPlaneThicknessNm > 0.)
      fState.geometryValid = fState.geometryValid &&
        std::abs(thicknessNm-fState.expectedGroundPlaneThicknessNm) < 1e-6 &&
        thinFilmVolumes == 0;
    if (!fState.geometryValid)
      G4Exception("SmokeRunAction", "SmokeGeometry", FatalException,
                  "Ground-plane material, thickness, or patterned-film thickness does not match the macro");
  }

  class SmokeRunAction : public G4UserRunAction
  {
  public:
    explicit SmokeRunAction(SmokeState& state) : fState(state) {}
    void BeginOfRunAction(const G4Run*) override
    {
      G4cout << "G4CMP_RECORD_MIN_E_TRACKS value="
             << (G4CMPConfigManager::RecordMinETracks() ? "true" : "false") << G4endl;
      if (G4CMPConfigManager::RecordMinETracks())
        G4Exception("SmokeRunAction", "SmokeConfiguration", FatalException,
                    "This workflow requires /g4cmp/recordMinETracks false");

      CheckGeometry(fState);
    }
  private:
    SmokeState& fState;
  };

  class SmokeEventAction : public G4UserEventAction
  {
  public:
    explicit SmokeEventAction(SmokeState& state) : fState(state) {}
    void BeginOfEventAction(const G4Event*) override
    {
      fState.nextQPSampleTime.clear();
    }
    void EndOfEventAction(const G4Event* event) override
    {
      fState.aborted = fState.aborted || event->IsAborted();
      if (!event->IsAborted()) ++fState.completedEvents;
      G4cout << "SMOKE_EVENT event=" << event->GetEventID()
             << " aborted=" << (event->IsAborted() ? "true" : "false") << G4endl;
    }
  private:
    SmokeState& fState;
  };

  class SmokeSteppingAction : public G4UserSteppingAction
  {
  public:
    explicit SmokeSteppingAction(SmokeState& state) : fState(state) {}
    void UserSteppingAction(const G4Step* step) override
    {
      auto* track = step->GetTrack();
      const auto name = track->GetDefinition()->GetParticleName();
      const auto number = track->GetCurrentStepNumber();
      const auto* pre = step->GetPreStepPoint();
      const auto* post = step->GetPostStepPoint();
      const auto preVolume = pre->GetPhysicalVolume() ? pre->GetPhysicalVolume()->GetName() : "none";
      const auto postVolume = post->GetPhysicalVolume() ? post->GetPhysicalVolume()->GetName() : "none";
      const G4bool qp = name == "BogoliubovQP";
      if (track->GetParentID() == 0)
      {
        fState.primaryEdep += step->GetTotalEnergyDeposit();
        if (number == 1)
        {
          fState.primary["volume"] = preVolume;
          fState.primary["material"] = pre->GetMaterial()->GetName();
          G4cout << "SMOKE_PRIMARY " << fState.primary.dump() << G4endl;
        }
      }
      if (name.find("phonon") != std::string::npos && number == 1)
      {
        ++fState.phononTracks;
        if (!fState.phononReported)
        {
          G4cout << "SMOKE_PHONON volume=" << preVolume << G4endl;
          fState.phononReported = true;
        }
      }
      if (qp && number == 1)
      {
        ++fState.qpTracks;
        if (!fState.qpReported)
        {
          G4cout << "SMOKE_QP_CREATED volume=" << preVolume << G4endl;
          fState.qpReported = true;
        }
      }
      if (qp && number > 1 && (post->GetPosition()-pre->GetPosition()).mag() > nm)
      {
        fState.movingQPTracks.insert(track->GetTrackID());
        if (!fState.movementReported)
        {
          G4cout << "SMOKE_QP_MOVED track=" << track->GetTrackID() << G4endl;
          fState.movementReported = true;
        }
      }
      if (qp && ++fState.qpStepsTotal % 100000 == 0)
      {
        // Heartbeat so a slow run can be told apart from a hung one.
        G4cout << "SMOKE_PROGRESS qp_steps=" << fState.qpStepsTotal << " track="
               << track->GetTrackID() << " track_step=" << number << " time_ns="
               << post->GetGlobalTime()/ns << " volume=" << postVolume << G4endl;
      }
      if (qp && fState.qpSampleIntervalNs > 0.) RecordQPSamples(step);
      if (qp && fState.maxQPSteps > 0 && number >= fState.maxQPSteps && track->GetTrackStatus() == fAlive)
      {
        ++fState.limitedQPTracks;
        track->SetTrackStatus(fStopAndKill);
      }
      if (number == 1 || (qp && (fState.recordAllQPSteps ||
          (fState.qpSampleIntervalNs <= 0. && number == 2))) ||
          track->GetTrackStatus() == fStopAndKill)
      {
        const auto prePosition = pre->GetPosition()/mm;
        const auto postPosition = post->GetPosition()/mm;
        const auto preDirection = pre->GetMomentumDirection();
        const auto postDirection = post->GetMomentumDirection();
        const auto preMaterial = pre->GetMaterial() ? pre->GetMaterial()->GetName() : "none";
        const auto postMaterial = post->GetMaterial() ? post->GetMaterial()->GetName() : "none";
        const auto* process = post->GetProcessDefinedStep();
        if (qp) ++fState.qpStepRows;
        std::ostringstream row;
        row << std::setprecision(15)
          << G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID() << ','
          << track->GetTrackID() << ',' << track->GetParentID() << ',' << number << ',' << name << ','
          << pre->GetKineticEnergy()/eV << ',' << post->GetKineticEnergy()/eV << ','
          << step->GetTotalEnergyDeposit()/eV << ',' << pre->GetGlobalTime()/ns << ','
          << post->GetGlobalTime()/ns << ',' << prePosition.x() << ',' << prePosition.y() << ','
          << prePosition.z() << ',' << postPosition.x() << ',' << postPosition.y() << ','
          << postPosition.z() << ',' << preDirection.x() << ',' << preDirection.y() << ','
          << preDirection.z() << ',' << postDirection.x() << ',' << postDirection.y() << ','
          << postDirection.z() << ',' << preMaterial << ',' << postMaterial << ',' << preVolume << ','
          << postVolume << ',' << (process ? process->GetProcessName() : "none") << ','
          << track->GetTrackStatus() << ',' << track->GetWeight() << ','
          << step->GetStepLength()/mm << '\n';
        fState.WriteSteps(row.str());
      }
    }
  private:
    void WriteQPSample(const G4Track* track, const G4String& kind, G4double time,
                       const G4ThreeVector& position, G4double energy,
                       const G4ThreeVector& direction, const G4String& material,
                       const G4String& volume, G4int status, G4int sourceStep)
    {
      std::ostringstream row;
      row << std::setprecision(15)
          << G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID() << ','
          << track->GetTrackID() << ',' << track->GetParentID() << ','
          << track->GetDefinition()->GetParticleName() << ',' << kind << ','
          << time/ns << ',' << position.x()/mm << ',' << position.y()/mm << ','
          << position.z()/mm << ',' << energy/eV << ',' << direction.x() << ','
          << direction.y() << ',' << direction.z() << ',' << material << ','
          << volume << ',' << status << ',' << track->GetWeight() << ',' << sourceStep << '\n';
      fState.WriteSamples(row.str());
      ++fState.qpSampleRows;
    }

    void RecordQPSamples(const G4Step* step)
    {
      const auto* track = step->GetTrack();
      const auto* pre = step->GetPreStepPoint();
      const auto* post = step->GetPostStepPoint();
      const auto number = track->GetCurrentStepNumber();
      const auto trackID = track->GetTrackID();
      const auto preMaterial = pre->GetMaterial() ? pre->GetMaterial()->GetName() : "none";
      const auto postMaterial = post->GetMaterial() ? post->GetMaterial()->GetName() : "none";
      const auto preVolume = pre->GetPhysicalVolume() ? pre->GetPhysicalVolume()->GetName() : "none";
      const auto postVolume = post->GetPhysicalVolume() ? post->GetPhysicalVolume()->GetName() : "none";

      if (number == 1)
      {
        WriteQPSample(track, "birth", pre->GetGlobalTime(), pre->GetPosition(),
                      pre->GetKineticEnergy(), pre->GetMomentumDirection(),
                      preMaterial, preVolume, fAlive, number);
        const auto interval = fState.qpSampleIntervalNs * ns;
        const auto firstBin = std::floor(pre->GetGlobalTime()/interval) + 1.;
        fState.nextQPSampleTime[trackID] = firstBin * interval;
      }

      auto sample = fState.nextQPSampleTime[trackID];
      const auto duration = post->GetGlobalTime() - pre->GetGlobalTime();
      while (sample <= post->GetGlobalTime())
      {
        const auto fraction = duration > 0. ? (sample-pre->GetGlobalTime())/duration : 1.;
        const auto position = pre->GetPosition() +
          fraction * (post->GetPosition()-pre->GetPosition());
        auto direction = pre->GetMomentumDirection() + fraction *
          (post->GetMomentumDirection()-pre->GetMomentumDirection());
        if (direction.mag2() > 0.) direction = direction.unit();
        const auto energy = pre->GetKineticEnergy() + fraction *
          (post->GetKineticEnergy()-pre->GetKineticEnergy());
        const auto atPost = fraction >= 1.;
        WriteQPSample(track, "sample", sample, position, energy, direction,
                      atPost ? postMaterial : preMaterial,
                      atPost ? postVolume : preVolume, fAlive, number);
        sample += fState.qpSampleIntervalNs * ns;
      }
      fState.nextQPSampleTime[trackID] = sample;

      if (track->GetTrackStatus() != fAlive)
      {
        WriteQPSample(track, "terminal", post->GetGlobalTime(), post->GetPosition(),
                      post->GetKineticEnergy(), post->GetMomentumDirection(),
                      postMaterial, postVolume, track->GetTrackStatus(), number);
        fState.nextQPSampleTime.erase(trackID);
      }
    }

    SmokeState& fState;
  };

  class SmokeActions : public G4VUserActionInitialization
  {
  public:
    explicit SmokeActions(SmokeState& state) : fState(state) {}
    void Build() const override
    {
      SetUserAction(new SmokeGenerator(fState));
      SetUserAction(new G4CMPStackingAction);
      SetUserAction(new SmokeRunAction(fState));
      SetUserAction(new SmokeEventAction(fState));
      SetUserAction(new SmokeSteppingAction(fState));
    }
  private:
    SmokeState& fState;
  };
}

namespace QArray
{
  // Write the upward-facing faces of every placed volume in global mm, one JSON
  // object per line, so analysis plots can draw the as-built chip layout.
  void DumpGeometryTopFaces(const G4String& path)
  {
    std::ofstream out(path);
    auto* world = G4TransportationManager::GetTransportationManager()
      ->GetNavigatorForTracking()->GetWorldVolume();
    std::function<void(const G4VPhysicalVolume*, const G4AffineTransform&, G4int)> walk =
      [&](const G4VPhysicalVolume* pv, const G4AffineTransform& parent, G4int depth)
    {
      const G4AffineTransform local(pv->GetRotation(), pv->GetTranslation());
      G4AffineTransform global;
      global.Product(local, parent);
      const auto* logical = pv->GetLogicalVolume();
      if (depth > 0)
      {
        auto* polyhedron = logical->GetSolid()->CreatePolyhedron();
        for (G4int face = 1; polyhedron && face <= polyhedron->GetNoFacets(); ++face)
        {
          const auto normal = global.TransformAxis(G4ThreeVector(
            polyhedron->GetNormal(face).x(), polyhedron->GetNormal(face).y(),
            polyhedron->GetNormal(face).z())).unit();
          if (normal.z() < 0.99) continue;
          G4int n = 0;
          G4Point3D nodes[4];
          polyhedron->GetFacet(face, n, nodes);
          nlohmann::json polygon = nlohmann::json::array();
          G4double z = 0.;
          for (G4int i = 0; i < n; ++i)
          {
            const auto p = global.TransformPoint(G4ThreeVector(nodes[i].x(), nodes[i].y(), nodes[i].z()));
            polygon.push_back({p.x()/mm, p.y()/mm});
            z = p.z()/mm;
          }
          out << nlohmann::json{{"volume", pv->GetName()}, {"material", logical->GetMaterial()->GetName()},
                                {"depth", depth}, {"z_mm", z}, {"polygon", polygon}}.dump() << '\n';
        }
        delete polyhedron;
      }
      for (std::size_t i = 0; i < logical->GetNoDaughters(); ++i)
        walk(logical->GetDaughter(i), global, depth+1);
    };
    walk(world, G4AffineTransform(), 0);
  }
}

namespace QArray
{
  // The stock example builds each resonator and the feedline on its own Al
  // "base" volume embedded in the ground plane. Those bases are the same
  // continuous film, but G4CMP's QP transport treats the Al-Al volume boundary
  // asymmetrically. With mergeConnectedAl, every Al volume placed directly in
  // the ground plane is dissolved into it: its daughters (gaps, pads) are moved
  // into the ground plane with unchanged global placement, and its Al-vacuum
  // border surfaces are re-created against the ground plane.
  class MergedAlDetectorConstruction : public QuasiparticleDetectorConstruction
  {
  public:
    explicit MergedAlDetectorConstruction(SmokeState& state) : fState(state) {}
    G4VPhysicalVolume* Construct() override
    {
      auto* world = QuasiparticleDetectorConstruction::Construct();
      if (fState.mergeConnectedAl) Merge();
      return world;
    }
  private:
    void Merge()
    {
      auto* plane = G4PhysicalVolumeStore::GetInstance()->GetVolume("GroundPlane", false);
      if (!plane) G4Exception("MergedAlDetectorConstruction", "SmokeMerge", FatalException,
                              "No GroundPlane volume to merge into");
      auto* planeLog = plane->GetLogicalVolume();
      const auto* planeMat = planeLog->GetMaterial();
      G4int dissolved = 0, moved = 0, borders = 0;
      nlohmann::json names = nlohmann::json::array();
      for (G4bool changed = true; changed;)
      {
        changed = false;
        for (std::size_t i = 0; i < planeLog->GetNoDaughters(); ++i)
        {
          auto* base = planeLog->GetDaughter(i);
          auto* baseLog = base->GetLogicalVolume();
          if (baseLog->GetMaterial() != planeMat) continue;
          const auto baseRot = base->GetObjectRotationValue();
          const auto baseTrans = base->GetObjectTranslation();
          std::vector<G4VPhysicalVolume*> children;
          for (std::size_t j = 0; j < baseLog->GetNoDaughters(); ++j)
            children.push_back(baseLog->GetDaughter(j));
          for (auto* child : children)
          {
            const G4RotationMatrix rot = baseRot * child->GetObjectRotationValue();
            const G4ThreeVector trans = baseRot * child->GetObjectTranslation() + baseTrans;
            baseLog->RemoveDaughter(child);
            child->SetMotherLogical(planeLog);
            child->SetRotation(rot.isIdentity() ? nullptr : new G4RotationMatrix(rot.inverse()));
            child->SetTranslation(trans);
            planeLog->AddDaughter(child);
            ++moved;
            // Carry the base's surface with this child over to the ground plane.
            for (const auto& [from, to, a, b] : {std::tuple{base, child, plane, child},
                                                 std::tuple{child, base, child, plane}})
            {
              auto* surface = G4CMPLogicalBorderSurface::GetSurface(from, to);
              if (!surface || G4CMPLogicalBorderSurface::GetSurface(a, b)) continue;
              new G4CMPLogicalBorderSurface(a->GetName()+"_merged_"+b->GetName(), a, b,
                                            surface->GetSurfaceProperty());
              ++borders;
            }
          }
          planeLog->RemoveDaughter(base);
          names.push_back(base->GetName());
          ++dissolved;
          changed = true;
          break;
        }
      }
      // Every remaining ground-plane daughter must have a defined surface both ways.
      nlohmann::json missing = nlohmann::json::array();
      for (std::size_t i = 0; i < planeLog->GetNoDaughters(); ++i)
      {
        auto* d = planeLog->GetDaughter(i);
        const auto* in = G4CMPLogicalBorderSurface::GetSurface(plane, d);
        const auto* out = G4CMPLogicalBorderSurface::GetSurface(d, plane);
        if (!in || !out || in->GetSurfaceProperty() != out->GetSurfaceProperty())
          missing.push_back({d->GetName(), d->GetLogicalVolume()->GetMaterial()->GetName(),
                             in != nullptr, out != nullptr});
      }
      fState.mergeReport = {{"dissolved_al_volumes", dissolved}, {"moved_daughters", moved},
                            {"daughters_missing_ground_plane_border", missing},
                            {"borders_created", borders}, {"dissolved_names", names},
                            {"ground_plane_daughters", planeLog->GetNoDaughters()}};
      G4cout << "SMOKE_MERGE_AL " << fState.mergeReport.dump() << G4endl;
    }
    SmokeState& fState;
  };
}

int main(int argc, char** argv)
{
  if (argc != 2 && argc != 4) { G4cerr << "usage: qp_example_smoke macro.mac [seed1 seed2]\n"; return EXIT_FAILURE; }
  const auto start = std::chrono::steady_clock::now();
  QArray::SmokeState state;
  if (!state.StepsReady()) { G4cerr << "Cannot write step output\n"; return EXIT_FAILURE; }
  G4GenericMessenger messenger(&state, "/smoke/", "Bounded transport validation");
  messenger.DeclareProperty("maxQPSteps", state.maxQPSteps);
  messenger.DeclareProperty("dumpGeometry", state.geometryDumpFile);
  messenger.DeclareProperty("mergeConnectedAl", state.mergeConnectedAl);
  messenger.DeclareProperty("requireQPs", state.requireQPs);
  messenger.DeclareProperty("recordAllQPSteps", state.recordAllQPSteps);
  messenger.DeclareProperty("qpSampleIntervalNs", state.qpSampleIntervalNs);
  messenger.DeclareProperty("expectedMaterial", state.expectedMaterial);
  messenger.DeclareProperty("expectedGroundPlaneMaterial", state.expectedGroundPlaneMaterial);
  messenger.DeclareProperty("expectedGroundPlaneThicknessNm", state.expectedGroundPlaneThicknessNm);
  G4CMPConfigManager::Instance();
  QuasiparticleConfigManager::Instance();
  auto* manager = new G4RunManager;
  manager->SetUserInitialization(new QArray::MergedAlDetectorConstruction(state));
  G4PhysListFactory factory;
  auto* physics = factory.GetReferencePhysList("QBBC_EMV");
  physics->SetVerboseLevel(0);
  physics->RegisterPhysics(new G4CMPPhysics);
  manager->SetUserInitialization(physics);
  manager->SetUserInitialization(new QArray::SmokeActions(state));
  // Optional "macro [seed1 seed2]": macros read the seeds as {seed1} and {seed2}.
  G4String seed1 = argc > 2 ? argv[2] : "10854";
  G4String seed2 = argc > 3 ? argv[3] : "4824";
  auto* ui = G4UImanager::GetUIpointer();
  ui->ApplyCommand("/control/alias seed1 "+seed1);
  ui->ApplyCommand("/control/alias seed2 "+seed2);
  const auto status = ui->ApplyCommand(G4String("/control/execute ")+argv[1]);
  if (status == 0 && !state.geometryDumpFile.empty())
  {
    QArray::CheckGeometry(state);
    QArray::DumpGeometryTopFaces(state.geometryDumpFile);
  }
  const G4bool passed = status == 0 && !state.aborted && state.geometryValid &&
    (state.expectedMaterial.empty() || state.primary.value("material", G4String()) == state.expectedMaterial) &&
    (!state.requireQPs || (state.qpTracks > 0 && !state.movingQPTracks.empty()));
  nlohmann::json summary = {{"passed", passed}, {"ui_status", status}, {"aborted", state.aborted},
    {"completed_events", state.completedEvents}, {"primary", state.primary},
    {"ground_plane", state.groundPlane}, {"merge_connected_al", state.mergeConnectedAl},
    {"merge_report", state.mergeReport},
    {"primary_edep_eV", state.primaryEdep/eV}, {"phonon_tracks", state.phononTracks},
    {"qp_tracks", state.qpTracks}, {"moving_qp_tracks", state.movingQPTracks.size()},
    {"qp_step_rows", state.qpStepRows}, {"record_all_qp_steps", state.recordAllQPSteps},
    {"qp_sample_interval_ns", state.qpSampleIntervalNs},
    {"qp_sample_rows", state.qpSampleRows},
    {"limited_qp_tracks", state.limitedQPTracks}, {"max_qp_steps", state.maxQPSteps},
    {"record_min_e_tracks", G4CMPConfigManager::RecordMinETracks()}, {"geant4_version", G4Version},
    {"g4cmp_source_tag", QARRAY_G4CMP_TAG}, {"g4cmp_source_commit", QARRAY_G4CMP_COMMIT},
    {"steps_file", state.steps.Path()}, {"qp_samples_file", state.samples.Path()},
    {"macro", argv[1]}, {"seed1", seed1}, {"seed2", seed2},
    {"sampling_energy_eV", G4CMPConfigManager::GetSamplingEnergy()/eV},
    {"phonon_bounces", G4CMPConfigManager::GetMaxPhononBounces()},
    {"qp_bounces", G4CMPConfigManager::GetMaxQPBounces()},
    {"elapsed_seconds", std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()}};
  state.FlushSteps();
  std::ofstream("summary.json") << summary.dump(2) << '\n';
  G4cout << "SMOKE_SUMMARY " << summary.dump() << G4endl;
  delete manager;
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
