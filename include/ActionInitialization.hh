#ifndef QRActionInitialization_h
#define QRActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

/// Action initialization class.

namespace QArray
{
  class ActionInitialization : public G4VUserActionInitialization
  {
  public:
    ActionInitialization() = default;
    virtual ~ActionInitialization() override = default;

    virtual void BuildForMaster() const override;
    virtual void Build() const override;
  };
}

#endif
