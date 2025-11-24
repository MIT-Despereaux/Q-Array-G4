#ifndef QRTrackingAction_hh
#define QRTrackingAction_hh 1

#include "G4UserTrackingAction.hh"
#include "HitData.hh"
#include "DataWriter.hh"

namespace QR
{
    class SensitiveDetector;
    
    class TrackingAction : public G4UserTrackingAction
    {
    public:
        TrackingAction(DataWriter *writer);
        virtual ~TrackingAction();

        virtual void PreUserTrackingAction(const G4Track *);
        virtual void PostUserTrackingAction(const G4Track *);
        // virtual void BeginOfEventAction();

        // inline SensitiveDetector *GetPrimaryDetector() const
        // {
        //     return mDataWriter->GetPrimaryDetector();
        // }
        // inline SensitiveDetector *GetRDecayDetector() const
        // {
        //     return mDataWriter->GetRDecayDetector();
        // }

    protected:
        DataWriter *mDataWriter;
    };
}

#endif
