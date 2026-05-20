#include "EventAction.hh"
#include "HitData.hh"
#include "Metadata.hh"

#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4SDManager.hh"
#include "G4RunManager.hh"

#include <exception>

namespace QArray
{
  inline EventAction::timept getms()
  {
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
  }

  json parseasvec(G4UIcommand *, const G4String &newValue)
  {
    std::stringstream ss(newValue);
    G4int entry;
    char sep;
    json result;
    ss >> sep;
    if (sep != '[')
      throw std::invalid_argument("json lists must start with '['");
    while (ss >> entry)
    {
      result.push_back(entry);
      ss >> sep;
      if (sep == ',')
        continue;
      if (sep == ']')
        break;
      else
      {
        throw std::invalid_argument("json lists must be separated by ','");
      }
    }
    return result;
  }

  EventAction::EventAction(DataWriter *mWriter)
      : G4UserEventAction(), mWriter(mWriter),
        iReportProgress(0), bRunInitialized(false), startTime(getms())
  {
    auto meta = Metadata::GetInstance();
    meta->AddParamCommand<G4UIcmdWithAString>("/QR/event/keepEvents",
                                              "Store list of events for viewing",
                                              "list",
                                              parseasvec);
    meta->Set("/QR/event/keepEvents", json(json::LIST));

    meta->AddParamCommand<G4UIcmdWithAnInteger>("/QR/event/printProgress",
                                                "Print estimated time remaining every N events", "N");
    meta->Set("/QR/event/printProgress", 0);
  }

  EventAction::~EventAction()
  {
  }

  void EventAction::BeginOfEventAction(const G4Event *event)
  {
    if (!bRunInitialized)
    {
      auto meta = Metadata::GetInstance();
      iReportProgress = meta->Get<long>("/QR/event/printProgress");

      const jsonvec &list = meta->Get<jsonvec>("/QR/event/keepEvents");
      for (const json &entry : list)
      {
        keptEvents.insert(entry.get<long>());
      }
      bRunInitialized = true;
    }

    // Keep the primary vertex in the event action class
    for (G4int i = 0; i < event->GetNumberOfPrimaryVertex(); ++i)
    {
      vecPrimaryVertices.push_back(event->GetPrimaryVertex(i));
    }

    mWriter->EventStart(event);
  }

  void EventAction::EndOfEventAction(const G4Event *event)
  {
    if (iReportProgress)
    {
      G4int eventID = event->GetEventID();
      if (eventID > 0 && (eventID % iReportProgress) == 0)
      {
        G4int ntot = G4RunManager::GetRunManager()->GetNumberOfEventsToBeProcessed();
        timept end = getms();
        double timedelta((end.time_since_epoch().count() - startTime.time_since_epoch().count()) / 1000.);
        double pct = (100. * eventID) / ntot;
        printf("\r                                                                                ");
        printf("\r %4.1f%% complete. Processing Time (sec): %6.1f Time Remaining (sec): %6.1f ",
               pct, timedelta, timedelta * (ntot / (eventID + 1.) - 1.));
        fflush(stdout);
      }
    }

    if (keptEvents.count(event->GetEventID()))
    {
      G4EventManager::GetEventManager()->KeepTheCurrentEvent();
    }
    mWriter->EventEnd(event);
  }
}
