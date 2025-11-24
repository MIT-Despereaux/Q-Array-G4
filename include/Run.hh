#ifndef QRRun_h
#define QRRun_h 1

// This custom run class is used to store custom accumulables.

#include "DetectorConstruction.hh"

#include "G4Run.hh"
#include "G4Event.hh"

#include <vector>

namespace QR
{
    struct EventUtils;
    using DC = class DetectorConstruction;

    class Run : public G4Run
    {
    public:
        Run();
        virtual ~Run() override;

        // Note that RecordEvent is processed after EventAction::EndOfEventAction
        virtual void RecordEvent(const G4Event *event) override;
        virtual void Merge(const G4Run *run) override;

        // Getters
        std::vector<G4double> GetDet1Energy() const { return fDet1Energy; }
        std::vector<G4double> GetDet2Energy() const { return fDet2Energy; }

    private:
        // std::vector<G4double> fDet1Energy = std::vector(DC::GetNDet1(), 0.0);
        std::vector<G4double> fDet1Energy = std::vector(2, 0.0);
        // std::vector<G4double> fDet2Energy = std::vector(DC::GetNDet2(), 0.0);
        std::vector<G4double> fDet2Energy = std::vector(2, 0.0);

        EventUtils *fEventUtils;
    };

}

#endif
