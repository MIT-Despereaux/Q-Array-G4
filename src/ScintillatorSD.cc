#include "ScintillatorSD.hh"

#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"

namespace QR
{

  ScintillatorSD::ScintillatorSD(const G4String &name)
      : G4VSensitiveDetector(name)
  {
    collectionName.insert("ScintillatorHitsCollection");
  }

  ScintillatorSD::ScintillatorSD(const G4String &name,
                                 const G4String &hitsCollectionName)
      : G4VSensitiveDetector(name)
  {
    collectionName.insert(hitsCollectionName);
  }

  void ScintillatorSD::Initialize(G4HCofThisEvent *hce)
  {
    // Create hits collection
    fHitsCollection = new ScintillatorHitsCollection(SensitiveDetectorName, collectionName[0]);

    // Add this collection in hce
    if (fHCID < 0)
    {
      fHCID = GetCollectionID(0);
    }
    hce->AddHitsCollection(fHCID, fHitsCollection);
  }

  G4bool ScintillatorSD::ProcessHits(G4Step *aStep,
                                     G4TouchableHistory *)
  {
    // energy deposit, in MeV
    G4double edep = aStep->GetTotalEnergyDeposit();

    if (edep == 0.)
      return false;

    auto newHit = new ScintillatorHit();

    newHit->SetTrackID(aStep->GetTrack()->GetTrackID());
    newHit->SetEdep(edep / MeV);
    newHit->SetCopyID(aStep->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber());
    newHit->SetPos(aStep->GetPostStepPoint()->GetPosition());

    fHitsCollection->insert(newHit);

    // newHit->Print();

    return true;
  }

  void ScintillatorSD::EndOfEvent(G4HCofThisEvent *)
  {
    if (verboseLevel > 1)
    {
      G4int nofHits = fHitsCollection->entries();
      G4cout << G4endl
             << "-------->Hits Collection: in this event there are " << nofHits
             << " hits in the Scintillator chambers. " << G4endl;
      // for (G4int i = 0; i < nofHits; i++)
      //   (*fHitsCollection)[i]->Print();
    }
  }
}
