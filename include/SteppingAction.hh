#ifndef QRSteppingAction_h
#define QRSteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "G4LogicalVolume.hh"
#include "globals.hh"

/// Stepping action class
///

namespace QArray
{
  class EventAction;

  class SteppingAction : public G4UserSteppingAction
  {
  public:
    void SetKillLowEnergyPhonons(G4bool flag) { fKillLowEnergyPhonons = flag; }
    SteppingAction(EventAction *eventAction);
    ~SteppingAction() override = default;

    // method from the base class
    void UserSteppingAction(const G4Step *) override;

  private:
    EventAction *fEventAction = nullptr;
    G4LogicalVolume *fScoringVolume = nullptr;
    G4bool fKillLowEnergyPhonons;
  };

}

#endif
