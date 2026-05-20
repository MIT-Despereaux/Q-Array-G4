#ifndef QRScintillatorSD_h
#define QRScintillatorSD_h 1

#include "ScintillatorHit.hh"

#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include "G4HCofThisEvent.hh"

namespace QArray
{

  /// Scintillator sensitive detector class
  ///
  /// The hits are accounted in hits in ProcessHits() function which is called
  /// by Geant4 kernel at each step. A hit is created with each step with non zero
  /// energy deposit.

  class ScintillatorSD : public G4VSensitiveDetector
  {
  public:
    ScintillatorSD(const G4String &name);
    ScintillatorSD(const G4String &name,
                   const G4String &hitsCollectionName);
    ~ScintillatorSD() override = default;

    // methods from base class
    void Initialize(G4HCofThisEvent *hce) override;
    G4bool ProcessHits(G4Step *step, G4TouchableHistory *history) override;
    void EndOfEvent(G4HCofThisEvent *hce) override;

  private:
    ScintillatorHitsCollection *fHitsCollection = nullptr;
    G4int fHCID = -1;
  };

}

#endif
