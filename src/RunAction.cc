#include "RunAction.hh"
#include "Metadata.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4CsvAnalysisManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

namespace QArray
{
  RunAction::RunAction() : G4UserRunAction(),
                           mPGen(nullptr), mWriter(new DataWriter())
  {
  }

  RunAction::RunAction(PrimaryGeneratorAction *pgen) : RunAction()
  {
    mPGen = pgen;
  }

  RunAction::~RunAction()
  {
    delete mWriter;
  }

  void RunAction::BeginOfRunAction(const G4Run *run)
  {
    if (isMaster)
    {
      // load the random state from the uuid seed
      // std::string uuidstr = Metadata::GetInstance()->GetString(seedkey);
      // // parse and convert to a long array
      // uuid_t uuidseed;
      // uuid_parse(uuidstr.c_str(), uuidseed);
      // G4Random::setTheSeeds((long *)uuidseed);

      // save Rndm status
      // G4RunManager::GetRunManager()->SetRandomNumberStore(false);
      G4Random::showEngineStatus();
    }

    // initialize primary generator
    if (mPGen)
      mPGen->BeginOfRunAction();

    // set up output data
    mWriter->RunStart(run, isMaster);
  }

  void RunAction::EndOfRunAction(const G4Run *run)
  {
    // end primary generator
    if (mPGen)
      mPGen->EndOfRunAction();

    // save output data
    mWriter->RunEnd(run, isMaster);

    // show Rndm status
    if (isMaster)
      G4Random::showEngineStatus();
  }

}
