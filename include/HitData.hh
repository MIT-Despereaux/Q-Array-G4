#ifndef QRHitData_hh
#define QRHitData_hh 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

#include <unordered_map>

class G4Track;
class G4Step;
class G4VAnalysisManager;

namespace QArray
{
  class HitData : public G4VHit
  {
  public:
    enum REDUCELEVEL
    {
      kMiminal = 0,
      kFull,
    };

    static std::unordered_map<std::string, REDUCELEVEL> const csRLVLTABLE;

  public:
    HitData(const G4Track *track = 0, const G4Step *step = 0,
            bool bLocalCoords = false, G4int iCopyNo = -1);
    virtual ~HitData();

    virtual void Draw();

    // must define memory allocation using G4Allocator
    inline void *operator new(size_t);
    inline void operator delete(void *);

    // Fill data from the current Track
    virtual void Fill(const G4Track *track, const G4Step *step = 0);

    // helper functions for G4AnalysisManager use
    static void DefineNtuple(G4VAnalysisManager *AMan, G4int iNtupleID,
                             REDUCELEVEL reduce = kMiminal,
                             bool bFinish = true);

    static bool bsWriteDecayAncestors;
    static bool bsIncludeNonIonizingEdep;

    virtual void FillNtuple(G4VAnalysisManager *AMan, G4int iNtupleID,
                            REDUCELEVEL reduce = kMiminal,
                            bool bFinish = true);

    // reduce hits
    HitData operator+(const HitData &right) const;
    HitData &operator+=(const HitData &right);

    // sort hits
    inline bool operator<(const HitData &right) const
    { // sort first by decay ancestry, then time
      if (decayAncestor && right.decayAncestor &&
          decayAncestor != right.decayAncestor)
        return decayAncestor->t0 < right.decayAncestor->t0;
      // if (decayDaughter && right.decayDaughter &&
      //     decayDaughter != right.decayDaughter)
      //   return decayDaughter->t0 < right.decayDaughter->t0;
      return t0 < right.t0 || (t0 == right.t0 && t1 < right.t1);
    }

    // Hash hits
    G4int hash() const
    {
      if (!this)
      {
        return 0;
      }
      G4int hash = 17;
      hash = hash * 31 + eventID;
      hash = hash * 31 + trackID;
      hash = hash * 31 + stepNum;
      hash = hash * 31 + particleCode;
      hash = hash * 31 + parentTrackID;
      hash = hash * 31 + copyno;
      hash = hash * 31 + bFilled;
      return hash;
    }

  public: // direct access to data members
    // ID information
    G4int eventID;
    G4int trackID;
    G4int stepNum;
    G4int particleCode;
    G4String particleName;
    G4int parentTrackID;       //< Parent track ID
    G4String incidentParticle; //< Primary particle name that first enters the detector
    G4int hashID;

    // Initial vertex information
    G4ThreeVector vx;
    G4ThreeVector vp;
    G4String primaryVertexParticle;

    // step interaction information
    G4int nHits; //< number of steps if reduced hit
    G4double edep;
    G4double edepER;
    G4double edepNR;
    G4double ke;
    G4double t0; //< in ns
    G4double t1;
    G4ThreeVector x0;
    G4ThreeVector x1;
    G4ThreeVector p0;
    G4ThreeVector p1;
    G4int boundary; //< is this a boundary crossing? 0=no, 1=pre, 2=post, 3=both
    G4double dx;
    G4String volname;
    G4String volnameNext;
    G4int copyno;
    G4String creatorProcess;
    G4String stepProcess;

    bool bWriteLocalCoords; //< do we write the local coordinates?
    G4ThreeVector local_x0;
    G4ThreeVector local_x1;
    G4ThreeVector local_p0;
    G4ThreeVector local_p1;

    // radioactive decay information
    HitData *decayAncestor;
    G4int decayAncestorHashID;
    HitData *decayDaughter;

    G4bool bFilled; //< has Fill ever been called?
  };

  typedef G4THitsCollection<HitData> QRHitsCollection;

  // Data and memory management
#if defined G4DIGI_ALLOC_EXPORT
  extern G4DLLEXPORT G4ThreadLocal G4Allocator<HitData> *HitDataAllocator;
#else
  extern G4DLLIMPORT G4ThreadLocal G4Allocator<HitData> *HitDataAllocator;
#endif

  // Allocation operators should be inlined for efficiency

  inline void *HitData::operator new(size_t)
  {
    if (!HitDataAllocator)
      HitDataAllocator = new G4Allocator<HitData>;
    return (void *)HitDataAllocator->MallocSingle();
  }

  inline void HitData::operator delete(void *aHit)
  {
    HitDataAllocator->FreeSingle((HitData *)aHit);
  }
}

#endif
