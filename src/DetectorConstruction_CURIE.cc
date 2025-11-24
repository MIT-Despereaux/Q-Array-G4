//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//  History :
//     21/10/2021 : DLa update to modify the material and the size
//     24/11/2025 : CURIE Maybell Fridge built from 
//                  Examples/Advanced/Microelectronics by
//                  Caitlyn Stone-Whitehead
// -------------------------------------------------------------------
// -------------------------------------------------------------------

#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "G4SystemOfUnits.hh"
#include "G4Region.hh"
#include "G4ProductionCuts.hh"
#include "G4RunManager.hh"
#include <G4SubtractionSolid.hh>
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

DetectorConstruction::DetectorConstruction()
:G4VUserDetectorConstruction(), fPhysiWorld(nullptr), fLogicWorld(nullptr),    fSolidWorld(nullptr),  fWorldMaterial(nullptr),  flogicTarget(nullptr),  ftargetSolid(nullptr),  fMaterial(nullptr),   fRegion(nullptr),  fCu(nullptr)
{
  fBoxSize = 1.0*um;
  fPhysiWorld = nullptr;
  fPhysiMXP = nullptr;
  fLogicWorld = nullptr;
  fSolidWorld = nullptr;
  fWorldMaterial = nullptr;
  flogicTarget = nullptr;
  VacuumChamber = nullptr;
  flogicVC = nullptr;
  VacuumChamberCoat = nullptr;
  flogicCoat = nullptr;
  stageSTP = nullptr;
  stageMXP = nullptr;
  flogicG = nullptr;
  flogicY = nullptr;
  flogicMXP = nullptr;
  flogicPbShield = nullptr;
  flogicSTP = nullptr;
  flogicICP = nullptr;

  DefineMaterials();
 // SetMaterial("G4_Si");  Use with G4Microelec
  fDetectorMessenger = new DetectorMessenger(this);
}  

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

DetectorConstruction::~DetectorConstruction()
{
  delete fPhysiWorld;
  delete fPhysiMXP;
  delete fLogicWorld;  
  delete fSolidWorld;  
  delete VacuumChamber;
  delete flogicVC;
  delete VacuumChamberCoat;
  delete flogicCoat;
  delete stageSTP;
  delete stageMXP;
  delete flogicG;
  delete flogicY;
  delete flogicMXP;
  delete flogicPbShield;
  delete flogicSTP;
  //delete fWorldMaterial;   // no delete because link to database
  delete flogicTarget;
  delete ftargetSolid;
  delete fMaterial;
  delete fCu;
  delete fPb;
  delete VacuumChamber0; 
  delete fRegion;
 // delete fShield;
  delete fDetectorMessenger;
  delete flogicICP;
  delete fSS;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4VPhysicalVolume* DetectorConstruction::Construct()

{
  DefineMaterials();
  return ConstructDetector();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void DetectorConstruction::DefineMaterials()
{ 

  // Silicon is defined from NIST material database
  G4NistManager * man = G4NistManager::Instance();
  fMaterial =  man->FindOrBuildMaterial("G4_Si");
  fCu =  man->FindOrBuildMaterial("G4_Cu");
  fWorldMaterial =  man->FindOrBuildMaterial("G4_AIR");  // world material;
  fPb = man->FindOrBuildMaterial("G4_Pb");
  fSS = man->FindOrBuildMaterial("G4_STAINLESS-STEEL");
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....



void DetectorConstruction::SetMaterial(const G4String& materialChoice)
{
  // search the material by its name
  G4Material* pttoMaterial = 
     G4NistManager::Instance()->FindOrBuildMaterial(materialChoice);
  
  if (pttoMaterial) {
    fMaterial = pttoMaterial;
    if ( flogicTarget ) { flogicTarget->SetMaterial(fMaterial); }
  } else {
    G4cout << "\n--> warning from DetectorConstruction::SetMaterial : "
           << materialChoice << " not found" << G4endl;
  }
  G4RunManager::GetRunManager()->PhysicsHasBeenModified();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....
G4VPhysicalVolume* DetectorConstruction::ConstructDetector()
{


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

  // WORLD VOLUME
  
  
  G4double TargetSizeX =  1*cm; 
  G4double TargetSizeY =  TargetSizeX; 
  G4double TargetSizeZ =  TargetSizeX; 
  fBoxSize = TargetSizeX;

  fWorldSizeX  = 2*5.3*m; 
  fWorldSizeY  = 2*5.3*m; 
  fWorldSizeZ  = 2*5.3*m; 

  fSolidWorld = new G4Box("World",                                 //its name
                                 fWorldSizeX/2,fWorldSizeY/2,fWorldSizeZ/2);   //its size


  fLogicWorld = new G4LogicalVolume(fSolidWorld,	//its solid
				   fWorldMaterial,		//its material
				   "World");		//its name
  
  fPhysiWorld = new G4PVPlacement(0,			//no rotation
  				 G4ThreeVector(),	//at (0,0,0)
                                 "World",		//its name
                                 fLogicWorld,		//its logical volume
                                 0,			//its mother  volume
                                 false,			//no boolean operation
                                 0);			//copy number



  ftargetSolid = new G4Box("Target",				     //its name
				 TargetSizeX/2,TargetSizeY/2,TargetSizeZ/2);   //its size
  

  flogicTarget = new G4LogicalVolume(ftargetSolid,       //its solid
						     fMaterial,	//its material
						     "Target");		//its name

  new G4PVPlacement(0,                                                 //no rotation
                   G4ThreeVector(0,0,-fWorldSizeZ/2+111.18*cm/2.+15.24*mm+6.65*mm+1.016*mm+1.016*mm+1.016*mm),                                   //at (0,0,0)
                   "Target",           //its name
                   flogicTarget,       //its logical volume
                   fPhysiWorld,                //its mother  volume
                   false,              //no boolean operation
                   0);                 //copy number


  G4Box *outerVC = new G4Box("Outer Vacuum Chamber Box",68.29*cm/2.,55.88*cm/2.,164.95*cm/2.);
  G4Box *innerVC = new G4Box("Inner Vacuum Chamber Box",(68.29*cm-2*8.89*mm)/2.,(55.88*cm-2*8.89*mm)/2.,(164.95*cm-15.24*mm-31.75*mm)/2.);
  G4SubtractionSolid *VacuumChamber0 = new G4SubtractionSolid("Vacuum Chamber",outerVC,innerVC,0,G4ThreeVector(0,0,-8.255*mm));

  flogicVC = new G4LogicalVolume(VacuumChamber0,       //its solid
                                                     fCu, //its material
                                                     "Vacuum Chamber");         //its name
  new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+164.95*cm/2.),                                   //at (0,0,0)
                    "Vacuum Chamber",           //its name
                    flogicVC,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number
  
  G4Box *innerCoat = new G4Box("Inner Vacuum Chamber Coating Box",((68.29*cm-2*8.95*mm)-2*6.35*mm)/2.,((55.88*cm-2*8.95*mm)-2*6.35*mm)/2.,((148.18*cm-2*15.5*mm)-2*6.35*mm)/2.);;
  G4Box *outerCoat = new G4Box("Outer Vacuum Chamber Coating Box",(68.29*cm-2*8.95*mm)/2.,(55.88*cm-2*8.95*mm)/2.,(148.18*cm-15.5*mm)/2.);
  G4SubtractionSolid *VacuumChamberCoat = new G4SubtractionSolid("Vacuum Chamber Coating",outerCoat,innerCoat,0,G4ThreeVector(0,0,6.35*mm/2));
  
  flogicCoat = new G4LogicalVolume(VacuumChamberCoat,       //its solid
                                                     fCu, //its material
                                                     "Vacuum Chamber Coating");         //its name
  new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+(148.18*cm)/2.+15.25*mm),                                   //at (0,0,0)
                    "Vacuum Chamber Coating",           //its name
                    flogicCoat,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number
  
  
  G4Box *outerG = new G4Box("Outer PRP Box",57.02*cm/2.,45.83*cm/2.,140.62*cm/2.);
  G4Box *innerG = new G4Box("Inner PRP Box",(57.02*cm-1.016*mm*2)/2.,(45.83*cm-1.016*mm*2)/2.,(140.62*cm-2*9.525*mm)/2.);
  G4SubtractionSolid *stageG = new G4SubtractionSolid("PRP Stage",outerG,innerG);


  flogicG = new G4LogicalVolume(stageG,       //its solid
                                                     fSS, //its material
                                                     "PRP Stage");         //its name
  new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+(145.18*cm)/2.+15.25*mm+6.35*mm),                                   //at (0,0,0)
                    "PRP Stage",           //its name
                    flogicG,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number


  G4Box *outerY = new G4Box("Outer RGP Box",53.96*cm/2.,42.91*cm/2.,109.99*cm/2.);
  G4Box *innerY = new G4Box("Inner RGP Box",(53.96*cm-1.016*2*mm)/2.,(42.91*cm-1.016*2*mm)/2.,(109.99*cm-2*9.525*mm)/2.);
  G4SubtractionSolid *stageY = new G4SubtractionSolid("RGP Stage",outerY,innerY);


  flogicY = new G4LogicalVolume(stageY,       //its solid
                                                     fSS, //its material
                                                     "RGP Stage");         //its name
  new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+120*cm/2.+15.24*mm+6.35*mm+1.016*mm),                                   //at (0,0,0)
                    "RGP Stage",           //its name
                    flogicY,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number

  
  G4Box *outerSTP = new G4Box("Outer STP + CF Plate Box",51.37*cm/2.,39.91*cm/2.,93.60*cm/2.);
  G4Box *innerSTP = new G4Box("Inner STP + CF Plate Box",(51.37*cm-2*1.016*mm)/2.,(39.91*cm-2*1.016*mm)/2.,(93.60*cm-1.016*mm-2*9.525*mm)/2.);
  G4SubtractionSolid *stageSTP = new G4SubtractionSolid("STP Stage",outerSTP,innerSTP,0,G4ThreeVector(0,0,-9.017*mm/2));


  flogicSTP = new G4LogicalVolume(stageSTP,       //its solid
                                                     fCu, //its material
                                                    "STP Stage + CF Plate");         //its name
  new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+111.18*cm/2+15.24*mm+6.65*mm+1.016*mm+1.016*mm),                                   //at (0,0,0)
                    "STP Stage + CF Plate",           //its name
                    flogicSTP,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number

  G4Box *ICPplate = new G4Box("ICP Plate",(48.32*cm-2*1.016*mm)/2.,(37.13*cm-2*1.016*mm)/2.,(9.525*mm)/2.);
  flogicICP = new G4LogicalVolume(ICPplate,       //its solid
                                                     fCu, //its material
                                                    "ICP Plate");         //its name
  new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+113.18*cm+15.24*mm+6.65*mm+1.016*mm+1.016*mm),                                   
                    "ICP Plate",           //its name
                    flogicICP,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number


  G4Box *outerMXP = new G4Box("Outer MXP + Cu Shield Box",48.32*cm/2.,37.13*cm/2.,58.40*cm/2.);
  G4Box *innerMXP = new G4Box("Inner MXP + Cu Shield Box",(48.32*cm-(2*1.016*mm+2*5*mm))/2.,(37.13*cm-(2*1.016*mm+2*5*mm))/2.,(58.4*cm-1.016*mm-9.525*mm)/2.);
  G4SubtractionSolid *stageMXP = new G4SubtractionSolid("MXP Stage + Cu Shield",outerMXP,innerMXP,0,G4ThreeVector(0,0,-4.25*mm/2));


  flogicMXP = new G4LogicalVolume(stageMXP,       //its solid
                                                      fCu, //its material
                                                     "MXP Stage + Cu Shield");         //its name
  fPhysiMXP = new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+111.18*cm/2.+15.24*mm+6.65*mm+1.016*mm+1.016*mm+1.016*mm),                                   //at (0,0,0)
                    "MXP Stage + Cu Shield",           //its name
                    flogicMXP,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number
 
  G4Box *PbShield = new G4Box("Shield Pb Top",25.4*cm/2.,10.16*cm/2.,5*mm/2.);
  
  flogicPbShield = new G4LogicalVolume(PbShield,       //its solid
                                                     fPb, //its material
                                                    "Shield Pb Top");         //its name
  new G4PVPlacement(0,                                                 //no rotation
                    G4ThreeVector(0,0,-fWorldSizeZ/2+111.18*cm/2.+15.24*mm+6.65*mm+1.016*mm+1.016*mm+1.016*mm+45.72*cm/2.+5*mm),
                    "Shield Pb Top",           //its name
                    flogicPbShield,       //its logical volume
                    fPhysiWorld,                //its mother  volume
                    false,              //no boolean operation
                    0);                 //copy number


  // Visualization attributes
  G4VisAttributes* worldVisAtt= new G4VisAttributes(G4Colour(1.0,1.0,1.0)); //White
  worldVisAtt->SetVisibility(true);
  fLogicWorld->SetVisAttributes(worldVisAtt);

  G4VisAttributes* worldVisAtt1 = new G4VisAttributes(G4Colour(1.0,0.0,0.0)); 
  worldVisAtt1->SetVisibility(true);
  flogicTarget->SetVisAttributes(worldVisAtt1);

//  G4VisAttributes* worldVisAtt2 = new G4VisAttributes(G4Colour(1.0,0.5,0.5));
//  worldVisAtt2->SetVisibility(true);
//  flogicVC->SetVisAttributes(worldVisAtt2);

//  G4VisAttributes* worldVisAtt3 = new G4VisAttributes(G4Colour(1.0,0.0,1.0));
//  worldVisAtt3->SetVisibility(true);
//  flogicCoat->SetVisAttributes(worldVisAtt3);

//  G4VisAttributes* worldVisAtt4 = new G4VisAttributes(G4Colour(1.0,1.0,0.0));
//  worldVisAtt4->SetVisibility(true);
//  flogicG->SetVisAttributes(worldVisAtt4);

//  G4VisAttributes* worldVisAtt5 = new G4VisAttributes(G4Colour(0.0,1.0,0.0));
//  worldVisAtt5->SetVisibility(true);
//  flogicY->SetVisAttributes(worldVisAtt5);

//  G4VisAttributes* worldVisAtt6 = new G4VisAttributes(G4Colour(0.0,0.0,1.0));
//  worldVisAtt6->SetVisibility(true);
//  flogicSTP->SetVisAttributes(worldVisAtt6);

//  G4VisAttributes* worldVisAtt7 = new G4VisAttributes(G4Colour(0.0,1.0,0.0));
//  worldVisAtt7->SetVisibility(true);
//  flogicMXP->SetVisAttributes(worldVisAtt7);

//  G4VisAttributes* worldVisAtt8 = new G4VisAttributes(G4Colour(0.0,1.0,0.0));
//  worldVisAtt8->SetVisibility(true);
//  flogicShield->SetVisAttributes(worldVisAtt8);



  // Create Target G4Region and add logical volume
  
  fRegion = new G4Region("Target");
//  fShield = new G4Region("Stage Shield");
  G4ProductionCuts* cuts = new G4ProductionCuts();
  
  G4double defCut = 1*nanometer;
  cuts->SetProductionCut(defCut,"gamma");
  cuts->SetProductionCut(defCut,"e-");
  cuts->SetProductionCut(defCut,"e+");
  cuts->SetProductionCut(defCut,"proton");
  
 // fShield->SetProductionCuts(cuts);
  fRegion->SetProductionCuts(cuts);
  fRegion->AddRootLogicalVolume(flogicTarget); 

  return fPhysiWorld;
}
 
 
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetSize(G4double value)
{
  fBoxSize = value;
  if(ftargetSolid) {
    ftargetSolid->SetXHalfLength(fBoxSize/2);
    ftargetSolid->SetYHalfLength(fBoxSize/2);
    ftargetSolid->SetZHalfLength(fBoxSize/2);
  }
  fWorldSizeX = value*2.0;
  fWorldSizeY = value*2.0;
  fWorldSizeZ = value*2.0;
  if(fSolidWorld) {
    fSolidWorld->SetXHalfLength(fWorldSizeX/2);
    fSolidWorld->SetYHalfLength(fWorldSizeY/2);
    fSolidWorld->SetZHalfLength(fWorldSizeZ/2);
  }  
  
}
