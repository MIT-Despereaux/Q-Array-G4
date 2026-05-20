#include "Run.hh"
#include "EventUtils.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4SDManager.hh"

namespace QArray
{
    Run::Run() : G4Run()
    {
        fEventUtils = new EventUtils();
    }

    Run::~Run()
    {
        delete fEventUtils;
    }

    void Run::RecordEvent(const G4Event *event)
    {
        G4Run::RecordEvent(event);

        fEventUtils->Reset();
        fEventUtils->RecordEvent(event);

        // Add the vector
        for (G4int i = 0; i < fEventUtils->fDet1Energy.size(); i++)
        {
            fDet1Energy[i] += fEventUtils->fDet1Energy[i];
        }
    }

    void Run::Merge(const G4Run *run)
    {
        const Run *localRun = static_cast<const Run *>(run);

        for (G4int i = 0; i < localRun->GetDet1Energy().size(); i++)
        {
            fDet1Energy[i] += localRun->GetDet1Energy()[i];
        }

        // Merge the accumulables
        G4Run::Merge(localRun);
    }
}
