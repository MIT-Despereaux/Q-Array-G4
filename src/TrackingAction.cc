#include "TrackingAction.hh"
#include "TrackInformation.hh"

#include "G4TrackingManager.hh"
#include "G4LatticeManager.hh"
#include "G4LatticePhysical.hh"  // Added to fix the incomplete type error!
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Threading.hh"
#include "G4SystemOfUnits.hh"     // Added to ensure all CLHEP units resolve smoothly
#include <cstdlib>
#include <filesystem>  // Requires C++17
#include <fstream>

namespace QArray
{
    TrackingAction::TrackingAction(DataWriter *writer) : G4UserTrackingAction(),
                                                         mDataWriter(writer)
    {
    }

    TrackingAction::~TrackingAction()
    {
    }

    void TrackingAction::PreUserTrackingAction(const G4Track *track)
    {
        // // --- DIAGNOSTIC BLOCK START ---
        // if (track && track->GetVolume()) {
        //     G4VPhysicalVolume* currentVol = track->GetVolume();
            
        //     // Filter output so it only prints when evaluating your target cube
        //     if (currentVol->GetName() == "DetectorSnCubePhysical") {
        //         auto* LM = G4LatticeManager::GetLatticeManager();
        //         G4bool hasLattice = LM->HasLattice(currentVol);
                
        //         G4cout << "[G4CMP-TRACKING-DIAGNOSTIC] Thread: " 
        //                << G4Threading::G4GetThreadId()
        //                << " | Particle: " << track->GetDefinition()->GetParticleName()
        //                << " | Volume Name: " << currentVol->GetName()
        //                << " | Pointer Address: " << currentVol
        //                << " | Has Registered Lattice? " << (hasLattice ? "YES" : "NO")
        //                << G4endl;
        //     }
        // }
        // // --- DIAGNOSTIC BLOCK END ---

        // --- PORTABLE SELF-HEALING LATTICE REGISTRATION ---
        if (track && track->GetVolume()) {
            G4VPhysicalVolume* currentVol = track->GetVolume();
            
            if (currentVol->GetName() == "DetectorSnCubePhysical") {
                auto* LM = G4LatticeManager::GetLatticeManager();
                
                if (!LM->HasLattice(currentVol)) {
                    G4LogicalVolume* logVol = currentVol->GetLogicalVolume();
                    G4LatticeLogical* latticeLogical = LM->LoadLattice(logVol->GetMaterial(), "Al");
                    
                    G4double milli_eV = 1e-3 * CLHEP::eV;
                    auto* crystalLattice = new G4LatticePhysical(
                        latticeLogical, 0.0, 0.18 * milli_eV, 0.01 * CLHEP::kelvin, 
                        1.22e11 / (milli_eV * CLHEP::mm3), 10.0 * CLHEP::ms, 100.0 * CLHEP::ns
                    );
                    
                    LM->RegisterLattice(currentVol, crystalLattice);
                }
            }
        }

        // Create the tracking information
        TrackInformation *trackInfo = static_cast<TrackInformation *>(track->GetUserInformation());
        if (!trackInfo)
        {
            trackInfo = new TrackInformation();
        }
        if (track->GetParentID() == 0)
        {
            // Primary
            trackInfo->FillOriginalTrack(track);
            track->SetUserInformation(trackInfo);
        }
        else
        {
            // Secondary
            trackInfo->FillCurrentTrack(track);
            track->SetUserInformation(trackInfo);
        }
    }

    void TrackingAction::PostUserTrackingAction(const G4Track *track)
    {
        // Copy the information from mother to daughter tracks
        G4TrackVector *secondaries = fpTrackingManager->GimmeSecondaries();
        if (secondaries)
        {
            TrackInformation *info =
                static_cast<TrackInformation *>(track->GetUserInformation());
            size_t nSec = secondaries->size();
            if (nSec > 0)
            {
                for (size_t i = 0; i < nSec; i++)
                {
                    TrackInformation *infoNew = new TrackInformation(info);
                    (*secondaries)[i]->SetUserInformation(infoNew);
                }
            }
        }
    }
}