#ifndef QRRunAction_h
#define QRRunAction_h 1

#include "DataWriter.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4UserRunAction.hh"
#include "G4Run.hh"
#include "globals.hh"

#include <vector>

namespace QArray
{
  class RunAction : public G4UserRunAction
  {
  public:
    RunAction();
    RunAction(PrimaryGeneratorAction *pGen);
    virtual ~RunAction() override;

    virtual void BeginOfRunAction(const G4Run *run) override;
    virtual void EndOfRunAction(const G4Run *run) override;

    DataWriter *GetDataWriter() const { return mWriter; }

  private:
    PrimaryGeneratorAction *mPGen;
    DataWriter *mWriter;
  };

}

#endif
