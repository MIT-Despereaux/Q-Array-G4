#ifndef QRSENSITIVEDETECTOR_h
#define QRSENSITIVEDETECTOR_h

#include "HitData.hh"

#include "G4VSensitiveDetector.hh"
#include "G4GenericMessenger.hh"

#include <set>

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

namespace QR
{
  class SensitiveDetector : public G4VSensitiveDetector
  {
  public:
    SensitiveDetector(G4String name = "Detector",
                      bool bIsreducible = true,
                      G4String HCName = "");
    virtual ~SensitiveDetector();

    // override methods from base class
    virtual void Initialize(G4HCofThisEvent *hitsCollection);
    virtual G4bool ProcessHits(G4Step *step, G4TouchableHistory *history);
    virtual void EndOfEvent(G4HCofThisEvent *hitsCollection);

    // add some additional helpful methods
    /// virtual method to accept/reject a track/step
    virtual bool Accept(const G4Step *step);
    /// add a hit directly
    virtual void AddHit(HitData *hit)
    {
      if (active && mHitCollection)
        mHitCollection->insert(hit);
    }

    QRHitsCollection *GetHitCollection() const { return mHitCollection; }

    /// Does it make sense to reduce hits in this detector?
    bool IsReducible() const { return bIsReducible; }
    void SetReducible(bool reducible = true) { bIsReducible = reducible; }

    /// Should we write local or global coordinates?
    bool GetWriteLocalCoordinates() const { return bWriteLocalCoords; }
    void SetWriteLocalCoordinates(bool local = true) { bWriteLocalCoords = local; }

    /// Should we use globally unique copy numbers?
    bool GetWriteDeepCopyNo() const { return bWriteDeepCopyNo; }
    void SetWriteDeepCopyNo(bool write = true) { bWriteDeepCopyNo = write; }

    /// Step energy deposit Threshold
    G4double GetStepTreshold() const { return fStepThreshold; }
    void SetStepThreshold(G4double threshold) { fStepThreshold = threshold; }

    /// Calculate the global copy number of this touchable
    G4int GetGlobalCopyNumber(const G4VTouchable *touchable) const;

  protected:
    // utilities for calculating global copy number
    struct PVPath
    {
      int copynum = -1;
      std::map<const G4VPhysicalVolume *, PVPath> branches;
      operator int() const { return copynum; }

      PVPath &operator[](const G4VPhysicalVolume *v) { return branches[v]; }
      const PVPath &operator[](const G4VPhysicalVolume *v) const { return branches.at(v); }

      PVPath &at(const G4VPhysicalVolume *v) { return branches.at(v); }
      const PVPath &at(const G4VPhysicalVolume *v) const { return branches.at(v); }

      PVPath(const PVPath &other) = default;
      PVPath &operator=(const PVPath &right) = default;

      PVPath(int n = -1) : copynum(n) {}
      PVPath &operator=(int n)
      {
        copynum = n;
        return *this;
      }
    };

    PVPath mGlobalCopyNumbers;

    void FindAllPhysicalVolumes(double &totalmass, double &totalarea);
    void SearchVolTree(std::vector<G4VPhysicalVolume *> &searchpath,
                       double &totalmass, double &totalarea);

    void DefineCommands();

  protected:
    QRHitsCollection *mHitCollection;
    G4GenericMessenger *mMessenger;

    // User cmd variables
    G4bool bIsReducible;
    G4double fStepThreshold;
    G4bool bWriteLocalCoords;
    G4bool bWriteDeepCopyNo;
  };
}

/// Detector that measure particles passing through boundary
// class FluxCounter : public SensitiveDetector
// {
// public:
//   FluxCounter(const G4String &name, bool killTracks = false,
//               const std::set<G4VPhysicalVolume *> &restrictvols = {});
//   virtual ~FluxCounter();

//   virtual bool Accept(const G4Step *step);
//   virtual G4bool ProcessHits(G4Step *step, G4TouchableHistory *history);

// protected:
//   bool fKillTracks;
//   std::set<G4VPhysicalVolume *> fRestrictVols;
// };

#endif
