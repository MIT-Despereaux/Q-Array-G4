#include "TrackInformation.hh"

#include "G4SystemOfUnits.hh"

namespace QArray
{
    G4ThreadLocal G4Allocator<TrackInformation> *
        TrackInformationAllocator = nullptr;

    TrackInformation::TrackInformation()
        : G4VUserTrackInformation()
    {
    }

    // Copy constructor
    TrackInformation::TrackInformation(const TrackInformation *trackInfo)
        : G4VUserTrackInformation()
    {
        iOriginalTrackID = trackInfo->iOriginalTrackID;
        particleDefinition = trackInfo->particleDefinition;
        mOriginalPosition = trackInfo->mOriginalPosition;
        mOriginalMomentum = trackInfo->mOriginalMomentum;
        dOriginalEnergy = trackInfo->dOriginalEnergy;
        dOriginalTime = trackInfo->dOriginalTime;

        iCurrentTrackID = trackInfo->iCurrentTrackID;
        currentDefinition = trackInfo->currentDefinition;
        mCurrentPosition = trackInfo->mCurrentPosition;
        mCurrentMomentum = trackInfo->mCurrentMomentum;
        dCurrentEnergy = trackInfo->dCurrentEnergy;
        dCurrentTime = trackInfo->dCurrentTime;
    }

    TrackInformation::~TrackInformation()
    {
    }

    // Assignment operator
    TrackInformation &TrackInformation ::operator=(const TrackInformation &trackInfo)
    {
        iOriginalTrackID = trackInfo.iOriginalTrackID;
        particleDefinition = trackInfo.particleDefinition;
        mOriginalPosition = trackInfo.mOriginalPosition;
        mOriginalMomentum = trackInfo.mOriginalMomentum;
        dOriginalEnergy = trackInfo.dOriginalEnergy;
        dOriginalTime = trackInfo.dOriginalTime;

        iCurrentTrackID = trackInfo.iCurrentTrackID;
        currentDefinition = trackInfo.currentDefinition;
        mCurrentPosition = trackInfo.mCurrentPosition;
        mCurrentMomentum = trackInfo.mCurrentMomentum;
        dCurrentEnergy = trackInfo.dCurrentEnergy;
        dCurrentTime = trackInfo.dCurrentTime;

        return *this;
    }

    void TrackInformation::FillOriginalTrack(const G4Track *track)
    {
        iOriginalTrackID = track->GetTrackID();
        particleDefinition = track->GetDefinition();
        mOriginalPosition = track->GetPosition();
        mOriginalMomentum = track->GetMomentum();
        dOriginalEnergy = track->GetTotalEnergy();
        dOriginalTime = track->GetGlobalTime();
    }

    void TrackInformation::FillCurrentTrack(const G4Track *track)
    {
        iCurrentTrackID = track->GetTrackID();
        currentDefinition = track->GetDefinition();
        mCurrentPosition = track->GetPosition();
        mCurrentMomentum = track->GetMomentum();
        dCurrentEnergy = track->GetTotalEnergy();
        dCurrentTime = track->GetGlobalTime();
    }

    void TrackInformation::Print() const
    {
        G4cout
            << "Original primary track ID " << iOriginalTrackID << " ("
            << particleDefinition->GetParticleName() << ","
            << dOriginalEnergy / GeV << "[GeV])" << G4endl;
    }

}
