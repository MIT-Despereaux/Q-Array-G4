#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "TrackingAction.hh"
#include "SteppingAction.hh"

namespace QR
{
  void ActionInitialization::BuildForMaster() const
  {
    SetUserAction(new RunAction);
  }

  void ActionInitialization::Build() const
  {
    PrimaryGeneratorAction *primary = new PrimaryGeneratorAction();
    SetUserAction(primary);

    RunAction *runAction = new RunAction(primary);
    SetUserAction(runAction);
    DataWriter *writer = runAction->GetDataWriter();

    EventAction *event = new EventAction(writer);
    SetUserAction(event);

    TrackingAction* trackingAction = new TrackingAction(writer);
    SetUserAction(trackingAction);

    // SetUserAction(new SteppingAction(event));
  }
}
