#include "PrimaryGeneratorAction.hh"
#include "Metadata.hh"
#include "json.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4VSolid.hh"
#include "G4RunManager.hh"
#include "G4GeneralParticleSource.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PhysicalVolumesSearchScene.hh"
#include "G4TransportationManager.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithABool.hh"

#include "CRYSetup.h"
#include "CRYGenerator.h"
#include "CRYParticle.h"

#include <sstream>

const std::string CRYCONFIGKEY = "/QR/generator/CRYconfig";

// function to parse CRY commands
json UpdateCRYSetting(G4UIcommand *, const G4String &newValue)
{
  // first, parse into key-value pair
  std::stringstream ss(newValue);
  std::string key, val;
  if (!(ss >> key >> val))
  {
    G4ExceptionDescription msg;
    msg << "UpdateCRYSetting Expected key-value pair";
    G4Exception("UpdateCRYSetting", "1", FatalException, msg);
  }
  // now get any existing settings
  auto meta = Metadata::GetInstance();
  json &allvals = meta->GetJSON(CRYCONFIGKEY);
  // and update with the new value
  allvals[key] = val;
  return allvals;
}

namespace QR
{
  typedef std::vector<G4PhysicalVolumesSearchScene::Findings> FindingsVector;

  PrimaryGeneratorAction::PrimaryGeneratorAction()
      : G4VUserPrimaryGeneratorAction()
  {
    G4int n_particle = 1;
    mParticleGun = new G4ParticleGun(n_particle);

    // Default particle kinematics
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4String particleName;
    G4ParticleDefinition *fMuon = particleTable->FindParticle(particleName = "mu+");
    mParticleGun->SetParticleDefinition(fMuon);
    mParticleGun->SetParticleEnergy(1000. * MeV);

    mGeneralParticleSource = new G4GeneralParticleSource();
    mSampler = new MCSampler();

    DefineCommands();
  }

  PrimaryGeneratorAction::~PrimaryGeneratorAction()
  {
    delete mGeneralParticleSource;
    delete mParticleGun;
    delete mMessenger;
    delete mCRYGen;
    delete mSampler;
  }

  void PrimaryGeneratorAction::GeneratePrimaries(G4Event *anEvent)
  {
    switch (mMode)
    {
    case kGPS:
      mGeneralParticleSource->GeneratePrimaryVertex(anEvent);
      break;
    case kParticleGun:
      // G4cout << "Current seed: " << G4Random::getTheSeed() << G4endl;
      InitCosmicParticleGun();
      mParticleGun->GeneratePrimaryVertex(anEvent);
      break;
    case kCRY:
      // Use CRY
      GenerateCRYEvent(anEvent);
      break;
    }
  }

  double randflat() { return G4UniformRand(); }

  const G4VPhysicalVolume *FindVolume(const G4String &volname, G4int copyno)
  {
    // see if this volume actually exists
    G4PhysicalVolumeStore *pvStore = G4PhysicalVolumeStore::GetInstance();
    const auto &iter =
        std::find_if(pvStore->begin(), pvStore->end(),
                     [&volname, copyno](const G4VPhysicalVolume *pv)
                     { return pv->GetName() == volname && (copyno < 0 || pv->GetCopyNo() == copyno); });
    if (iter == pvStore->end())
    {
      G4ExceptionDescription msg;
      msg << "No physical volume '" << volname << " (" << copyno << ")"
          << "' found.";
      G4Exception("PrimaryGeneratorAction::FindVolume", "1", FatalException, msg);
    }
    return *iter;
  }

  FindingsVector LocateAllVolumes(const G4String &volname, G4int copyno)
  {
    FindingsVector result;
    // get the world volume
    G4TransportationManager *tm = G4TransportationManager::GetTransportationManager();
    size_t nWorlds = tm->GetNoWorlds();
    if (nWorlds < 1)
    {
      G4ExceptionDescription msg;
      msg << "No world volume registered with transportation manager!";
      G4Exception("PrimaryGeneratorAction::LocateAllVolumes", "1", FatalException, msg);
    }
    size_t iWorld = 0;
    auto iterWorld = tm->GetWorldsIterator();
    G4VPhysicalVolume *pWorld = nullptr;
    for (size_t iWorld = 0; iWorld < nWorlds; iWorld++, iterWorld++)
    {
      // use vis tools to find the volumes
      pWorld = *iterWorld;
      G4PhysicalVolumeModel searchModel(pWorld); // Unlimited depth.
      G4ModelingParameters mp;                  // Default - no culling.
      searchModel.SetModelingParameters(&mp);
      G4PhysicalVolumesSearchScene searchScene(&searchModel, volname, copyno);
      searchModel.DescribeYourselfTo(searchScene); // Initiate search.
      for (const auto &finding : searchScene.GetFindings())
      {
        result.push_back(finding);
      }
    }
    return result;
  }

  G4ThreeVector LocateVolume(const G4String &volname,
                             G4int copyno)
  {
    // Get the list of all volumes and take the first one
    FindingsVector findings = LocateAllVolumes(volname, copyno);
    if (findings.empty())
    {
      G4ExceptionDescription msg;
      msg << "Volume " << volname << " is not found within world geometry!";
      G4Exception("PrimaryGeneratorAction::LocateVolume", "1", FatalException, msg);
    }
    // Prints out all the findings and their locations
    for (const auto &finding : findings)
    {
      G4cout << finding.fFoundBasePVPath
      << ' ' << finding.fpFoundPV->GetName() << ' ' << finding.fFoundPVCopyNo
      << " (mother logical volume: "
      << finding.fpFoundPV->GetMotherLogical()->GetName()
      << ')' << G4endl;
      G4cout << "Volume " << volname << " located at"
             << (finding.fFoundObjectTransformation.getTranslation()) / cm << " cm"
             << G4endl;
    }
    return findings[0].fFoundObjectTransformation.getTranslation();
  }

  G4Transform3D LocateVolume(const G4VPhysicalVolume *vol)
  {
    // todo: this will fail if multiple volumes have same names!
    return G4Transform3D(G4RotationMatrix(), LocateVolume(vol->GetName(), vol->GetCopyNo()));
  }

  void PrimaryGeneratorAction::BeginOfRunAction()
  {
    // create a settings string for CRY
    auto meta = Metadata::GetInstance();

    G4String mode = meta->GetString("/QR/generator/mode");
    if (mode == "gps")
      mMode = kGPS;
    else if (mode == "cry")
      mMode = kCRY;
    else if (mode == "particleGun")
      mMode = kParticleGun;
    else
    {
      G4ExceptionDescription msg;
      msg << "Unknown generator mode: " << mode;
      G4Exception("PrimaryGeneratorAction::BeginOfRunAction", "1",
                  FatalException, msg);
    }
    if (mMode == kCRY)
    {
      G4cout << "Initializing CRY cosmic ray generator..." << G4endl;
      std::stringstream ss;
      jsonmap settings = meta->Get<jsonmap>(CRYCONFIGKEY);
      for (auto &kv : settings)
      {
        ss << kv.first << " " << kv.second.get<std::string>() << " ";
      }
      mCRYOffset = meta->GetThreeVector("/QR/generator/CRYOffset");
      G4cout << "mCRYOffset = " << mCRYOffset << G4endl;
      ss << "xoffset " << mCRYOffset.x() / m 
      << " " << "yoffset " << mCRYOffset.y() / m 
      << " " << "zoffset " << mCRYOffset.z() / m << " ";
      G4cout << "CRY settings: \n"
             << ss.str() << G4endl;
      // now set up the generator
      CRYSetup setup(ss.str(), CRYDATA);
      setup.setRandomFunction(randflat);
      mCRYGen = new CRYGenerator(&setup);

      // set the seconds simulated to make sure it exists later
      meta->Set("/QR/generator/CRYTotalTime", 0.);
    }

    bApplyAcceptance = meta->Get<bool>("/QR/generator/acceptance/apply");
    if (bApplyAcceptance)
    {
      if (mMode != kCRY)
      {
        G4cerr << "PrimaryGeneratorAction WARNING: acceptance cuts are only "
               << "enabled for the 'cry' generator mode!" << G4endl;
      }
      // locate the requested volume
      G4String volname = meta->GetString("/QR/generator/acceptance/targetVolume");
      G4int copyno = meta->GetInt("/QR/generator/acceptance/targetCopyNo");
      const G4VPhysicalVolume *targetvol = FindVolume(volname, copyno);
      mAcceptanceTarget = LocateVolume(targetvol).getTranslation();
      mAcceptanceTarget += meta->GetThreeVector("/QR/generator/acceptance/targetOffset");

      // determine size of acceptance target
      G4double acceptR = meta->GetDouble("/QR/generator/acceptance/targetRadius");
      if (acceptR <= 0)
      {
        // use the target's bounding box
        const G4VSolid *solid = targetvol->GetLogicalVolume()->GetSolid();
        G4ThreeVector boundmin, boundmax;
        solid->BoundingLimits(boundmin, boundmax);
        acceptR = (boundmax - boundmin).mag() / 2.;
        meta->Set("/QR/generator/acceptance/targetRadius", acceptR);
        G4cout << "Primary acceptance radius = " << acceptR / m << " m" << G4endl;
      }
      fAcceptanceConeRsq = acceptR * acceptR;
    }
  }

  void PrimaryGeneratorAction::EndOfRunAction()
  {
    if (mCRYGen)
    {
      // write some statistics:
      // in case of multithreading, sim may be > 0:
      auto meta = Metadata::GetInstance();
      double sim = (mCRYGen->timeSimulated() +
                    meta->Get<double>("/QR/generator/CRYTotalTime"));
      meta->Set("/QR/generator/CRYTotalTime", sim);
      delete mCRYGen;
      mCRYGen = 0;
    }
  }

  void PrimaryGeneratorAction::GenerateCRYEvent(G4Event *anEvent)
  {
    mCRYGen->genEvent(&vecCRYParts);
    for (CRYParticle *part : vecCRYParts)
    {
      G4ThreeVector start = G4ThreeVector(part->x(), part->y(), part->z()) * m + mCRYOffset;
      G4ThreeVector momentum = G4ThreeVector(part->u(), part->v(), part->w());
      G4bool throwme = true;
      if (bApplyAcceptance)
      {
        G4ThreeVector totarget = mAcceptanceTarget - start;
        G4double costheta_accept = totarget.mag() /
                                   sqrt(fAcceptanceConeRsq + totarget.mag2());
        G4double costheta = totarget.dot(momentum) / (totarget.mag() * momentum.mag());

        throwme = (costheta > costheta_accept);
      }
      if (throwme)
      {
        G4PrimaryVertex *pv = new G4PrimaryVertex(start, 0);
        G4PrimaryParticle *particle = new G4PrimaryParticle(part->PDGid());
        particle->SetKineticEnergy(part->ke() * MeV);
        particle->SetMomentumDirection(momentum.unit());
        pv->SetPrimary(particle);
        anEvent->AddPrimaryVertex(pv);
      }
      delete part;
    }
    vecCRYParts.clear();
  }

  G4double GeneratePhi()
  {
    return G4UniformRand() * 2. * M_PI;
  }

  void GenerateThetaPhi(G4double &theta, G4double &phi, G4double thetaMin = 0., G4double thetaMax = 90. * deg)
  {
    // Check if thetaMin <= thetaMax
    if (thetaMin > thetaMax)
    {
      G4Exception("PrimaryGeneratorAction::GenerateThetaPhi()", "InvalidSetup", FatalException,
                  "thetaMin > thetaMax");
    }
    G4double u = G4UniformRand();
    G4double N = 1. / 3. * (std::pow(std::cos(thetaMin), 3) - std::pow(std::cos(thetaMax), 3));
    G4double tmp = std::pow(std::cos(thetaMin), 3) - 3 * u * N;
    theta = std::acos(std::pow(tmp, 1. / 3.));
    phi = GeneratePhi();
  }

  void GeneratePosition(G4ThreeVector &position, G4double sideL)
  {
    G4double x, y, z;
    z = 0.;
    // Generate uniform X, Y position in +/- sideL/2
    x = (G4UniformRand() - 0.5) * sideL;
    y = (G4UniformRand() - 0.5) * sideL;
    position = G4ThreeVector(x, y, z);
  }

  // Note energy must be in GeV
  G4double *Gaisser(G4double *energy, G4int size)
  {
    G4double *flux = new G4double[size];
    for (G4int i = 0; i < size; i++)
    {
      G4double CK = 0.054;
      G4double gamma = 2.7;
      G4double EPi = 115;
      G4double EK = 850;
      G4double en = energy[i];
      // G4cout << "Energy: " << en << G4endl;
      flux[i] = std::pow(en, -gamma) * (1. / (1. + (1.1 * en / EPi)) + 0.054 / (1. + (1.1 * en / EK)));
      // G4cout << "Flux: " << flux[i] << G4endl;
    }
    return flux;
  }

  G4double PrimaryGeneratorAction::GenerateEnergy()
  {
    // Check if fMinEnergy <= fMaxEnergy
    if (fMinEnergy > fMaxEnergy)
    {
      G4Exception("PrimaryGeneratorAction::GenerateEnergy()", "InvalidSetup", FatalException,
                  "fMinEnergy > fMaxEnergy");
    }
    if (!mRandGeneral)
    {
      G4int fSize = 1000;
      G4double *energy = new G4double[fSize];
      // Create the energy array from fMinEnergy to fMaxEnergy in fSize steps
      for (G4int i = 0; i < fSize; i++)
      {
        energy[i] = (fMinEnergy + i * (fMaxEnergy - fMinEnergy) / fSize) / GeV;
      }
      G4double *flux = Gaisser(energy, fSize);
      // Print the values
      // for (G4int i = 0; i < fSize; i++)
      // {
      //   G4cout << "Energy: " << energy[i] << " GeV, Flux: " << flux[i] << G4endl;
      // }
      mRandGeneral = new G4RandGeneral(flux, fSize);
      delete[] energy;
      delete[] flux;
    }
    G4double rand = mRandGeneral->shoot();
    // Transform to the correct energy range
    return fMinEnergy + rand * (fMaxEnergy - fMinEnergy);
  }

  void PrimaryGeneratorAction::GenerateMCThetaEnergy(G4double &theta, G4double &energy)
  {
    // Check if fMinEnergy <= fMaxEnergy
    if (fMinEnergy > fMaxEnergy)
    {
      G4Exception("PrimaryGeneratorAction::GenerateMCThetaEnergy()", "InvalidSetup", FatalException,
                  "fMinEnergy > fMaxEnergy");
    }
    // Check if thetaMin <= thetaMax
    if (fThetaMin > fThetaMax)
    {
      G4Exception("PrimaryGeneratorAction::GenerateMCThetaEnergy()", "InvalidSetup", FatalException,
                  "fThetaMin > fThetaMax");
    }
    // Initialize the sampler
    if (!(mSampler->bInitialized))
    {
      G4cout << "Initializing MCSampler..." << G4endl;
      mSampler->SetParameterE(fMinEnergy, fMaxEnergy);
      mSampler->SetParameterTheta(fThetaMin, fThetaMax);
      mSampler->Initialize();
    }
    mSampler->DrawOneSample(theta, energy, MCSampler::TYPE::RWMH);
  }

  void PrimaryGeneratorAction::DeInitSampler()
  {
    mSampler->bInitialized = false;
  }

  void PrimaryGeneratorAction::InitCosmicParticleGun()
  {
    G4double theta, phi, energy;
    G4double dirTheta, dirPhi;

    // Get pointer to the world volume
    G4LogicalVolume *worldLogical = G4LogicalVolumeStore::GetInstance()->GetVolume("worldLogical");

    // Get pointer to the world solid
    G4VSolid *worldSolid = worldLogical->GetSolid();
    G4double worldX = dynamic_cast<G4Box *>(worldSolid)->GetXHalfLength();
    // Set the radius R to 0.75 times half of the world X/Y/Z side length
    G4double R = 0.75 * worldX;

    // GenerateThetaPhi(theta, phi, fThetaMin, fThetaMax);
    phi = GeneratePhi();
    GenerateMCThetaEnergy(theta, energy);
    // G4cout << "Theta: " << theta << G4endl;
    // G4cout << "Phi: " << phi << G4endl;
    dirTheta = M_PI - theta;
    dirPhi = phi + M_PI;
    G4ThreeVector direction =
        G4ThreeVector(std::sin(dirTheta) * std::cos(dirPhi), std::sin(dirTheta) * std::sin(dirPhi), std::cos(dirTheta));

    G4ThreeVector planePos = G4ThreeVector();
    GeneratePosition(planePos, fSideL);
    G4double planeX, planeY;
    planeX = planePos.getX();
    planeY = planePos.getY();
    G4ThreeVector thetaHat = G4ThreeVector(std::cos(theta) * std::cos(phi), std::cos(theta) * std::sin(phi), -std::sin(theta));
    G4ThreeVector phiHat = G4ThreeVector(-std::sin(phi), std::cos(phi), 0.);
    G4ThreeVector startPos = G4ThreeVector(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
    startPos *= R;
    G4ThreeVector position = -planeY * thetaHat + planeX * phiHat + startPos;

    // Set the particle gun
    mParticleGun->SetParticlePosition(position);
    mParticleGun->SetParticleMomentumDirection(direction);
    // energy = GenerateEnergy();
    // G4cout << "Energy: [MeV] " << energy << G4endl;
    mParticleGun->SetParticleEnergy(energy);

    // Generate charge
    long seed = G4Random::getTheSeed();
    mEngineC.seed(static_cast<unsigned int>(seed));
    mCharge = (mDiscDistC(mEngineC) == 0) ? 1 : -1;
    mParticleGun->SetParticleCharge(mCharge);
    // G4cout << "Charge: " << mCharge << G4endl;
  }

  void PrimaryGeneratorAction::DefineCommands()
  {
    mMessenger = new G4GenericMessenger(this,
                                        "/QR/generator/",
                                        "Primary generator control");

    auto meta = Metadata::GetInstance();
    auto *gOffsetCmd = meta->AddParamCommand<G4UIcmdWith3VectorAndUnit>(
        "/QR/generator/CRYOffset", "CRY offset.");
    gOffsetCmd->SetUnitCategory("Length");
    meta->Set("/QR/generator/CRYOffset", G4ThreeVector(0, 0, 0));

    auto *genModeCmd = meta->AddParamCommand<G4UIcmdWithAString>(
        "/QR/generator/mode", "Generator mode.");
    genModeCmd->SetCandidates("particleGun gps cry");
    meta->Set("/QR/generator/mode", "cry");

    // register CRY config
    meta->AddParamCommand<G4UIcmdWithAString>(CRYCONFIGKEY,
                                              "Add a key-val to CRY setup",
                                              "key val",
                                              UpdateCRYSetting);
    meta->Set(CRYCONFIGKEY, json(json::OBJ));

    // variance reduction targeting (currently only works with CRY)
    meta->AddParamCommand<G4UIcmdWithABool>("/QR/generator/acceptance/apply",
                                            "Accept only primaries going toward a certain point?", "accept");
    meta->Set("/QR/generator/acceptance/apply", false);

    meta->AddParamCommand<G4UIcmdWithAString>("/QR/generator/acceptance/targetVolume",
                                              "Primaries must be headed toward this physical volume", "volname");
    meta->Set("/QR/generator/acceptance/targetVolume", "worldPhysical");

    meta->AddParamCommand<G4UIcmdWithAnInteger>("/QR/generator/acceptance/targetCopyNo",
                                                "Primaries must be headed toward this copy of the target volume", "copyNo");
    meta->Set("/QR/generator/acceptance/targetCopyNo", 0);

    auto *tOffsetCmd = meta->AddParamCommand<G4UIcmdWith3VectorAndUnit>(
        "/QR/generator/acceptance/targetOffset",
        "Offset the center of the acceptance cone");
    tOffsetCmd->SetUnitCategory("Length");
    meta->Set("/QR/generator/acceptance/targetOffset", G4ThreeVector(0, 0, 0));

    auto *tRadiusCmd = meta->AddParamCommand<G4UIcmdWithADoubleAndUnit>(
        "/QR/generator/acceptance/targetRadius",
        "Set the radius of the acceptance cone, or use bounding box if <=0",
        "rad");
    tRadiusCmd->SetUnitCategory("Length");
    meta->Set("/QR/generator/acceptance/targetRadius", 0);
 
    // SideL command
    G4String guidance = "Set the side length of the generating plane.";
    G4GenericMessenger::Command &sideLCmd = mMessenger->DeclarePropertyWithUnit("sideL", "m", fSideL);
    sideLCmd.SetGuidance(guidance);
    sideLCmd.SetParameterName("sideL", true);
    sideLCmd.SetRange("sideL>=0.");

    // ThetaMin command
    G4GenericMessenger::Command &thetaMinCmd = mMessenger->DeclarePropertyWithUnit("thetaMin", "deg", fThetaMin);
    guidance = "Set the minimum theta angle of the generating plane, in degrees. Default 0 deg.";
    thetaMinCmd.SetGuidance(guidance);
    thetaMinCmd.SetParameterName("thetaMin", true);
    thetaMinCmd.SetRange("thetaMin>=0. && thetaMin<=90.");

    // ThetaMax command
    G4GenericMessenger::Command &thetaMaxCmd = mMessenger->DeclarePropertyWithUnit("thetaMax", "deg", fThetaMax);
    guidance = "Set the maximum theta angle of the generating plane, in degrees. Default 90 deg.";
    thetaMaxCmd.SetGuidance(guidance);
    thetaMaxCmd.SetParameterName("thetaMax", true);
    thetaMaxCmd.SetRange("thetaMax>=0. && thetaMax<=90.");

    // MinEnergy command
    G4GenericMessenger::Command &minEnergyCmd = mMessenger->DeclarePropertyWithUnit("minEnergy", "GeV", fMinEnergy);
    guidance = "Set the minimum energy of the generated particles, in GeV.";
    minEnergyCmd.SetGuidance(guidance);
    minEnergyCmd.SetParameterName("minEnergy", true);
    minEnergyCmd.SetRange("minEnergy>=0.");

    // MaxEnergy command
    G4GenericMessenger::Command &maxEnergyCmd = mMessenger->DeclarePropertyWithUnit("maxEnergy", "GeV", fMaxEnergy);
    guidance = "Set the maximum energy of the generated particles, in GeV.";
    maxEnergyCmd.SetGuidance(guidance);
    maxEnergyCmd.SetParameterName("maxEnergy", true);
    maxEnergyCmd.SetRange("maxEnergy>=0.");

    // Initialize MCSampler
    G4GenericMessenger::Command &initSamplerCmd = mMessenger->DeclareMethod("deInitSampler", &PrimaryGeneratorAction::DeInitSampler);
    guidance = "De-initialize Sampler (MUST be done after changing parameter range).";

  }

}
