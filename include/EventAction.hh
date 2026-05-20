#ifndef QREventAction_h
#define QREventAction_h 1

#include "DataWriter.hh"

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "G4Event.hh"

#include <set>
#include <chrono>

namespace QArray
{
  class EventAction : public G4UserEventAction
  {
  public:
    using timept = std::chrono::time_point<std::chrono::system_clock,
                                           std::chrono::milliseconds>;

    EventAction(DataWriter *mWriter);
    virtual ~EventAction() override;

    virtual void BeginOfEventAction(const G4Event *event) override;
    virtual void EndOfEventAction(const G4Event *event) override;

  private:
    std::set<G4int> keptEvents;
    bool bRunInitialized;
    DataWriter *mWriter;
    timept startTime;
    int iReportProgress;

    std::vector<G4PrimaryVertex *> vecPrimaryVertices;
  };
}

#endif
