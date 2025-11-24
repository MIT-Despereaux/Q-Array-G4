#ifndef QRScintillatorHit_h
#define QRScintillatorHit_h 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "tls.hh" // For thread_local

namespace QR
{

  /// Scintillator Hit class: inheriting from G4VHit
  ///
  /// It defines data members to store the trackID, energy deposit,
  /// and position of charged particles in a selected volume:
  /// - fTrackID, fEdep, fPos

  class ScintillatorHit : public G4VHit
  {
  public:
    ScintillatorHit() = default;
    ScintillatorHit(const ScintillatorHit &) = default; // Copy constructor
    ~ScintillatorHit() override = default;

    // operators
    // G4bool operator==(const TrackerHit &) const;

    inline void *operator new(size_t);
    inline void operator delete(void *);

    // methods from base class
    void Draw() override;
    // void Print() override;

    // Set methods
    void SetTrackID(G4int track) { fTrackID = track; };
    void SetEdep(G4double eDep) { fEDep = eDep; };
    void SetPos(G4ThreeVector xyz) { fPos = xyz; };
    void SetCopyID(G4int copyID) { fCopyID = copyID; };

    // Get methods
    G4int GetTrackID() const { return fTrackID; };
    G4double GetEdep() const { return fEDep; };
    G4ThreeVector GetPos() const { return fPos; };
    G4int GetCopyID() const { return fCopyID; };

  private:
    G4int fTrackID = -1;
    G4double fEDep = 0.;
    G4ThreeVector fPos;
    G4int fCopyID = -1;
  };

  //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

  using ScintillatorHitsCollection = G4THitsCollection<ScintillatorHit>;

  extern G4ThreadLocal G4Allocator<ScintillatorHit> *ScintillatorHitAllocator;

  inline void *ScintillatorHit::operator new(size_t)
  {
    if (!ScintillatorHitAllocator)
      ScintillatorHitAllocator = new G4Allocator<ScintillatorHit>;
    return (void *)ScintillatorHitAllocator->MallocSingle();
  }

  inline void ScintillatorHit::operator delete(void *hit)
  {
    ScintillatorHitAllocator->FreeSingle((ScintillatorHit *)hit);
  }
}

#endif
