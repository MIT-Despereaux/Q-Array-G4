#include "TrackingAction.hh"
#include "TrackInformation.hh"

#include "G4TrackingManager.hh"

namespace QArray
{
    TrackingAction::TrackingAction(DataWriter *writer) : G4UserTrackingAction(),
                                                         mDataWriter(writer)
    {
    }

    TrackingAction::~TrackingAction()
    {
    }

    void TrackingAction::PreUserTrackingAction(const G4Track *track)
    {
        // Create the tracking information
        TrackInformation *trackInfo = static_cast<TrackInformation *>(track->GetUserInformation());
        if (!trackInfo)
        {
            trackInfo = new TrackInformation();
        }
        if (track->GetParentID() == 0)
        {
            // Primary
            trackInfo->FillOriginalTrack(track);
            track->SetUserInformation(trackInfo);
        }
        else
        {
            // Secondary
            trackInfo->FillCurrentTrack(track);
            track->SetUserInformation(trackInfo);
        }
    }

    void TrackingAction::PostUserTrackingAction(const G4Track *track)
    {
        // Copy the information from mother to daughter tracks
        G4TrackVector *secondaries = fpTrackingManager->GimmeSecondaries();
        if (secondaries)
        {
            TrackInformation *info =
                static_cast<TrackInformation *>(track->GetUserInformation());
            size_t nSec = secondaries->size();
            if (nSec > 0)
            {
                for (size_t i = 0; i < nSec; i++)
                {
                    TrackInformation *infoNew = new TrackInformation(info);
                    (*secondaries)[i]->SetUserInformation(infoNew);
                }
            }
        }
    }
}
