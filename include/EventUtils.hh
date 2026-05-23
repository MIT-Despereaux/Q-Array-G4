#include "ScintillatorHit.hh"
#if defined(QARRAY_DETECTOR_GEOMETRY_LEIDEN_II)
#include "DetectorConstruction_LEIDEN_II.hh"
#elif defined(QARRAY_DETECTOR_GEOMETRY_DSPX)
#include "DetectorConstruction_DSPX.hh"
#else
#include "DetectorConstruction_LEIDEN_II.hh"
#endif

#include "G4LogicalVolumeStore.hh"
#include "G4SDManager.hh"
#include "G4Event.hh"
#include "globals.hh"

#include <vector>

#ifndef QREventUtils_h
#define QREventUtils_h 1

namespace QArray
{
    using DC = class DetectorConstruction;

    struct EventUtils
    {
        // Vectors
        // std::vector<G4double> fDet1Energy = std::vector(DC::GetNDet1(), 0.0);
        std::vector<G4double> fDet1Energy = {0.0, 0.0};
        // std::vector<G4double> fDet2Energy = std::vector(DC::GetNDet2(), 0.0);
        std::vector<G4double> fDet2Energy = {0.0, 0.0};

        G4int fDet1HCID = -1;
        G4int fDet2HCID = -1;

        void Reset()
        {
            std::fill(fDet1Energy.begin(), fDet1Energy.end(), 0.0);
            std::fill(fDet2Energy.begin(), fDet2Energy.end(), 0.0);
        }

        void RecordEvent(const G4Event *event)
        {
            // Initialize hit collection IDs
            G4SDManager *sdManager = G4SDManager::GetSDMpointer();
            if (fDet1HCID < 0)
            {
                fDet1HCID = sdManager->GetCollectionID("detector1/ScintillatorHitsCollection");
                fDet2HCID = sdManager->GetCollectionID("detector2/ScintillatorHitsCollection");
            }

            // Process the event with its hit collections

            G4HCofThisEvent *hce = event->GetHCofThisEvent();
            if (!hce)
            {
                G4ExceptionDescription msg;
                msg << "No hits collection of this event found.\n";
                G4Exception("EventUtils::RecordEvent()",
                            "Code001", JustWarning, msg);
                return;
            }

            ScintillatorHitsCollection *det1HC = static_cast<ScintillatorHitsCollection *>(hce->GetHC(fDet1HCID));
            ScintillatorHitsCollection *det2HC = static_cast<ScintillatorHitsCollection *>(hce->GetHC(fDet2HCID));

            if (det1HC)
            {
                int n_hit = det1HC->entries();
                for (G4int i = 0; i < n_hit; i++)
                {
                    // Check which physical detector the hit belongs to
                    G4int copyNo = (*det1HC)[i]->GetCopyID();
                    // Add the energy to the vector
                    fDet1Energy[copyNo] += (*det1HC)[i]->GetEdep();
                }
            }
            if (det2HC)
            {
                int n_hit = det2HC->entries();
                for (G4int i = 0; i < n_hit; i++)
                {
                    G4int copyNo = (*det2HC)[i]->GetCopyID();
                    fDet2Energy[copyNo] += (*det2HC)[i]->GetEdep();
                }
            }
        }
    };
}

#endif
