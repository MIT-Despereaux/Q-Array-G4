#ifndef QRTrackInformation_h
#define QRTrackInformation_h 1

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4ParticleDefinition.hh"
#include "G4Track.hh"
#include "G4Allocator.hh"
#include "G4VUserTrackInformation.hh"

namespace QArray
{
    class TrackInformation : public G4VUserTrackInformation
    {
    public:
        TrackInformation();
        TrackInformation(const TrackInformation *trackInfo);
        virtual ~TrackInformation();

        inline void *operator new(size_t);
        inline void operator delete(void *trackInfo);

        TrackInformation &operator=(const TrackInformation &right);

        void FillOriginalTrack(const G4Track *track);
        void FillCurrentTrack(const G4Track *track);
        virtual void Print() const;

    public:
        // Information of the primary track at the primary vertex
        G4int iOriginalTrackID = -1; // Track ID of primary particle
        G4ParticleDefinition *particleDefinition = nullptr;
        G4ThreeVector mOriginalPosition = G4ThreeVector(0., 0., 0.);
        G4ThreeVector mOriginalMomentum = G4ThreeVector(0., 0., 0.);
        G4double dOriginalEnergy = 0; // Total energy KE + Mass
        G4double dOriginalTime = 0;

        // Information when the track was created
        G4int iCurrentTrackID = -1;
        G4ParticleDefinition *currentDefinition = nullptr;
        G4ThreeVector mCurrentPosition = G4ThreeVector(0., 0., 0.);
        G4ThreeVector mCurrentMomentum = G4ThreeVector(0., 0., 0.);
        G4double dCurrentEnergy = 0;
        G4double dCurrentTime = 0;
    };

    #if defined G4TRACK_ALLOC_EXPORT
    extern G4DLLEXPORT G4ThreadLocal G4Allocator<TrackInformation>* TrackInformationAllocator;
    #else
    extern G4DLLIMPORT G4ThreadLocal G4Allocator<TrackInformation>* TrackInformationAllocator;
    #endif

    inline void *TrackInformation::operator new(size_t)
    {
        if (!TrackInformationAllocator)
            TrackInformationAllocator = new G4Allocator<TrackInformation>;
        return (void *)TrackInformationAllocator->MallocSingle();
    }

    inline void TrackInformation::operator delete(void *trackInfo)
    {
        TrackInformationAllocator->FreeSingle((TrackInformation *)trackInfo);
    }
}

#endif
