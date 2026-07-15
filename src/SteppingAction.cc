#include "SteppingAction.hh"
#include "EventAction.hh"
#if defined(QARRAY_DETECTOR_GEOMETRY_LEIDEN_II)
#include "DetectorConstruction_LEIDEN_II.hh"
#elif defined(QARRAY_DETECTOR_GEOMETRY_DSPX)
#include "DetectorConstruction_DSPX.hh"
#else
#include "DetectorConstruction_LEIDEN_II.hh"
#endif

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

#include "Metadata.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh" 

namespace QArray
{
  SteppingAction::SteppingAction(EventAction *eventAction)
      : fEventAction(eventAction)
  {
  }

  void SteppingAction::UserSteppingAction(const G4Step *step)
  {
    if (!step) return;

    // -------------------------------------------------------------------------
    // 1. DYNAMIC MACRO QUERY VIA METADATA REGISTRY
    // -------------------------------------------------------------------------
    auto meta = Metadata::GetInstance();
    bool killLowEnergyPhonons = false;

    // Safely parse the parameter out of the nlohmann::json instance
    try 
    {
      killLowEnergyPhonons = meta->GetJSON("killLowEnergyPhonons").get<bool>();
    } 
    catch (...) 
    {
      killLowEnergyPhonons = false; // Fallback default if key isn't loaded yet
    }

    // -------------------------------------------------------------------------
    // 2. THE PHONON REAPER
    // -------------------------------------------------------------------------
    if (killLowEnergyPhonons)
    {
      G4Track* track = step->GetTrack();
      G4String particleName = track->GetDefinition()->GetParticleName();

      // Intercept any G4CMP custom acoustic phonon variations
      if (particleName.find("phonon") != std::string::npos)
      {
        // 0.35 meV matches the Cooper pair breaking threshold (2*Delta) for Aluminum
        if (track->GetKineticEnergy() < 0.35 * 1e-9 * MeV)
        {
          track->SetTrackStatus(fStopAndKill); // Drop particle from track collection stack
          return;                              // Halt further step handling immediately
        }
      }
    }

    // -------------------------------------------------------------------------
    // Legacy template scoring blocks (Left intact as requested)
    // -------------------------------------------------------------------------
    // if (!fScoringVolume)
    // {
    //   const auto detConstruction = static_cast<const DetectorConstruction *>(
    //       G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    //   fScoringVolume = detConstruction->GetScoringVolume();
    // }

    // if (abs(step->GetPreStepPoint()->GetPosition().z()) > 9900 ...
  }
}