#ifndef QRDataWriter_h
#define QRDataWriter_h 1

#include "HitData.hh"
#include "DataReducer.hh"
#include "SensitiveDetector.hh"
#include "json.hh"

#include "G4String.hh"

#include <vector>

class G4Run;
class G4Event;
class G4Track;

namespace QArray
{
    class DataWriter
    {
    public:
        DataWriter();
        ~DataWriter();

        void RunStart(const G4Run *run, bool bIsMaster);
        void RunEnd(const G4Run *run, bool bIsMaster);

        void EventStart(const G4Event *event);
        void EventEnd(const G4Event *event);

        // SensitiveDetector *GetPrimaryDetector() const { return fPrimaries; }
        // SensitiveDetector *GetRDecayDetector() const { return fRDecays; }
        // const std::vector<FateDetector *> &GetFateDetectors() const { return fFateDetectors; }

        /// Register a new way to reduce the data. Returns false if name is not unique
        bool RegisterHitMapping(const G4String &name, const DataMapper &mapping);

        /// Print registered mappings
        void PrintHitMappings();

        /// Register a new map reduce output
        bool RegisterReducedOutput(const G4String &name, const DataReducer &reducer);

        /// Create a new FateDetector
        // bool AddFateDetector(const G4String &name, const std::vector<G4String> &particles = {});

        /// Create a new FluxCounter
        // bool AddFluxCounter(const G4String &volname);

        /// Parse a macro command to add a new output
        json ParseNewOutputCmd(G4UIcommand *cmd, const G4String &cmdStr);

        json ParseReduceLvlCmd(G4UIcommand *cmd, const G4String &cmdStr);

        /// Parse a macro command to add new fate detector
        // json ParseNewFateDetectorCmd(G4UIcommand *cmd, const G4String &cmdStr);

    private:
        // Disable copy constructor and assignment operator
        DataWriter(const DataWriter &) = delete;
        DataWriter &operator=(const DataWriter &) = delete;

        std::map<G4String, DataMapper> mMappers;
        std::map<G4String, DataReducer> mReducers;
        DataReducer mAllStepsWriter;

        G4String fileName;
        G4bool bEnabled;
        G4bool bWriteSteps;
        G4bool bWriteAllEvents;
        G4bool bSort;

        std::vector<SensitiveDetector *> vecDetectors;
    };
}

#endif
