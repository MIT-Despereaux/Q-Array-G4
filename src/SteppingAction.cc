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

namespace QArray
{
  SteppingAction::SteppingAction(EventAction *eventAction)
      : fEventAction(eventAction)
  {
  }

  void SteppingAction::UserSteppingAction(const G4Step *step)
  {
    // if (!fScoringVolume)
    // {
    //   const auto detConstruction = static_cast<const DetectorConstruction *>(
    //       G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    //   fScoringVolume = detConstruction->GetScoringVolume();
    // }

    // Also print the prestep point coordinate if the z coordinate goes beyond limits
    // if (abs(step->GetPreStepPoint()->GetPosition().z()) > 9900 
    // || abs(step->GetPreStepPoint()->GetPosition().x()) > 9900
    // || abs(step->GetPreStepPoint()->GetPosition().y()) > 9900)
    // {
    //   G4cout << "PreStepPoint coordinate: " << step->GetPreStepPoint()->GetPosition() << G4endl;
    //   G4LogicalVolume *volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
    //   // Print out the volumne name
    //   G4cout << "Volume name: " << volume->GetName() << G4endl;
    // }

    // // check if we are in scoring volume
    // if (volume != fScoringVolume)
    //   return;

    // // collect energy deposited in this step
    // G4double edepStep = step->GetTotalEnergyDeposit();
    // fEventAction->AddEdep(edepStep);
  }
}
