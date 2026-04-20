#include "DetectorConstruction.hh"
#include "ScintSD.hh"
#include "OpticalSiPM_SD.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SDManager.hh"
#include "G4ProductionCuts.hh"
#include "G4Region.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4OpticalSurface.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4ios.hh"

#include <cmath>
#include <algorithm>

// ============================================================================
DetectorConstruction::DetectorConstruction()
    : G4VUserDetectorConstruction()
{
    DefineCommands();
}

DetectorConstruction::~DetectorConstruction()
{
    delete fMessenger;
}

void DetectorConstruction::DefineCommands()
{
    fMessenger = new G4GenericMessenger(this, "/scint/",
                                        "Configuracion del centellador");
    fMessenger->DeclareMethod("type",
        &DetectorConstruction::SetScintTypeByName,
        "Tipo de centellador: BGO | PLASTIC | CSI | LYSO")
        .SetParameterName("type", false);
}

void DetectorConstruction::SetScintTypeByName(const G4String& name)
{
    if      (name=="BGO"     || name=="bgo")     fScintType = ScintType::BGO;
    else if (name=="PLASTIC" || name=="plastic")  fScintType = ScintType::PLASTIC;
    else if (name=="CSI"     || name=="csi")      fScintType = ScintType::CSI;
    else if (name=="LYSO"    || name=="lyso")     fScintType = ScintType::LYSO;
    else { G4cerr<<"[Detector] Tipo desconocido: "<<name<<". Usando BGO.\n";
           fScintType = ScintType::BGO; }
}

// ── Propiedades ópticas ───────────────────────────────────────────────────────
void DetectorConstruction::AddOpticalProperties(G4Material* mat)
{
    const G4int N = 41;
    G4double energy[N], rindex[N], absl[N], comp1[N];

    G4double peakEv, sigmaEv, rindexVal, absLenCm, yieldPerMeV, tauNs;
    switch (fScintType) {
        case ScintType::BGO:
            peakEv=2.56*eV; sigmaEv=0.25*eV; rindexVal=2.15;
            absLenCm=25.; yieldPerMeV=8200.; tauNs=300.; break;
        case ScintType::PLASTIC:
            peakEv=2.92*eV; sigmaEv=0.20*eV; rindexVal=1.58;
            absLenCm=380.; yieldPerMeV=10000.; tauNs=2.1; break;
        case ScintType::CSI:
            peakEv=2.25*eV; sigmaEv=0.30*eV; rindexVal=1.80;
            absLenCm=100.; yieldPerMeV=54000.; tauNs=1000.; break;
        case ScintType::LYSO:
            peakEv=2.82*eV; sigmaEv=0.22*eV; rindexVal=1.82;
            absLenCm=40.; yieldPerMeV=32000.; tauNs=40.; break;
    }

    const G4double Emin=1.5*eV, Emax=4.1*eV;
    G4double maxC = 0.;
    for (G4int i=0; i<N; ++i) {
        energy[i] = Emin + (Emax-Emin)*(G4double(i)/(N-1));
        comp1[i]  = std::exp(-0.5*std::pow((energy[i]-peakEv)/sigmaEv,2.));
        rindex[i] = rindexVal;
        absl[i]   = absLenCm*cm;
        if (comp1[i]>maxC) maxC=comp1[i];
    }
    for (G4int i=0; i<N; ++i) comp1[i] /= maxC;

    auto* mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("RINDEX",                 energy, rindex, N);
    mpt->AddProperty("ABSLENGTH",              energy, absl,   N);
    mpt->AddProperty("SCINTILLATIONCOMPONENT1",energy, comp1,  N, true);
    mpt->AddConstProperty("SCINTILLATIONYIELD",         yieldPerMeV/MeV);
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", tauNs*ns);
    mpt->AddConstProperty("RESOLUTIONSCALE",            1.0);
    mpt->AddConstProperty("SCINTILLATIONYIELD1",        1.0);

    if (mat->GetIonisation()) {
        G4double birks = (fScintType==ScintType::PLASTIC) ? 0.126*mm/MeV
                                                           : 0.012*mm/MeV;
        mat->GetIonisation()->SetBirksConstant(birks);
    }
    mat->SetMaterialPropertiesTable(mpt);
}

G4Material* DetectorConstruction::BuildScintMaterial()
{
    auto* nist = G4NistManager::Instance();
    G4Material* mat = nullptr;
    switch (fScintType) {
        case ScintType::BGO:     mat = nist->FindOrBuildMaterial("G4_BGO");                      break;
        case ScintType::PLASTIC: mat = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");  break;
        case ScintType::CSI:     mat = nist->FindOrBuildMaterial("G4_CESIUM_IODIDE");            break;
        case ScintType::LYSO:    mat = nist->FindOrBuildMaterial("G4_LSO");                      break;
    }
    AddOpticalProperties(mat);
    return mat;
}

// ── Construct ────────────────────────────────────────────────────────────────
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto* nist = G4NistManager::Instance();

    // Mundo
    auto* airMat     = nist->FindOrBuildMaterial("G4_AIR");
    auto* solidWorld = new G4Box("World", 10*cm, 10*cm, 10*cm);
    auto* logicWorld = new G4LogicalVolume(solidWorld, airMat, "World");
    auto* physWorld  = new G4PVPlacement(nullptr,{},logicWorld,"World",nullptr,false,0);
    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    // Índice de refracción del aire
    {
        G4double eA[2]={1.5*eV,4.1*eV}, rA[2]={1.0,1.0};
        auto* mA = new G4MaterialPropertiesTable();
        mA->AddProperty("RINDEX",eA,rA,2);
        airMat->SetMaterialPropertiesTable(mA);
    }

    // Dimensiones del centellador
    G4double scintX,scintY,scintZ;
    if (fScintType==ScintType::PLASTIC) {
        scintX=4.7*cm; scintY=4.7*cm; scintZ=1.0*cm;
    } else {
        scintX=6.*mm; scintY=6.*mm; scintZ=15.*mm;
    }
    G4double scintHalfZ = scintZ/2.;
    G4double scintZpos  = scintHalfZ;   // cara de entrada en z=0

    auto* scintMat   = BuildScintMaterial();
    auto* solidScint = new G4Box("Scintillator",scintX/2.,scintY/2.,scintHalfZ);
    fLogicScint      = new G4LogicalVolume(solidScint,scintMat,"Scintillator");
    auto* physScint  = new G4PVPlacement(nullptr,{0,0,scintZpos},
                                          fLogicScint,"Scintillator",logicWorld,false,0);

    auto* visS = new G4VisAttributes(G4Colour(0.2,0.2,1.0,0.4));
    visS->SetForceSolid(true);
    fLogicScint->SetVisAttributes(visS);
    // Sin UserLimits: los pasos los controlan los procesos físicos.
    // G4UserLimits aplica a TODOS los tracks (incluidos fotones ópticos) y
    // con límite pequeño haría la simulación órdenes de magnitud más lenta.

    // Envoltura reflectante (Teflon, skin surface)
    {
        auto* sw = new G4OpticalSurface("WrapSurface");
        sw->SetType(dielectric_metal);
        sw->SetModel(unified);
        sw->SetFinish(ground);
        G4double eW[2]={1.5*eV,4.1*eV}, rW[2]={0.95,0.95}, efW[2]={0.,0.};
        auto* mW = new G4MaterialPropertiesTable();
        mW->AddProperty("REFLECTIVITY",eW,rW,2);
        mW->AddProperty("EFFICIENCY",  eW,efW,2);
        sw->SetMaterialPropertiesTable(mW);
        new G4LogicalSkinSurface("WrapSkin",fLogicScint,sw);
    }

    // SiPM
    G4double sipmZ    = 1.*mm;
    G4double sipmZpos = scintZpos + scintHalfZ + sipmZ/2.;
    auto* sipmMat   = nist->FindOrBuildMaterial("G4_Si");
    {
        // n_Si igualado al del BGO → reflexión de Fresnel = 0 % (acoplamiento óptico ideal)
        G4double eSi[2]={1.5*eV,4.1*eV}, rSi[2]={2.15,2.15}, aSi[2]={0.001*mm,0.001*mm};
        auto* mSi = new G4MaterialPropertiesTable();
        mSi->AddProperty("RINDEX",   eSi,rSi,2);
        mSi->AddProperty("ABSLENGTH",eSi,aSi,2);
        sipmMat->SetMaterialPropertiesTable(mSi);
    }
    auto* solidSiPM = new G4Box("SiPM",scintX/2.,scintY/2.,sipmZ/2.);
    fLogicSiPM      = new G4LogicalVolume(solidSiPM,sipmMat,"SiPM");
    auto* physSiPM  = new G4PVPlacement(nullptr,{0,0,sipmZpos},
                                         fLogicSiPM,"SiPM",logicWorld,false,0);

    auto* visSiPM = new G4VisAttributes(G4Colour(1.0,0.0,1.0,0.9));
    visSiPM->SetForceSolid(true);
    fLogicSiPM->SetVisAttributes(visSiPM);

    // Bloque de plomo detrás del SiPM: retrodispersa gammas que atraviesan el cristal
    // y genera el pico de backscattering (~184 keV para Cs-137 de 662 keV)
    G4double pbHalfZ = 2.*cm;
    G4double pbZpos  = sipmZpos + sipmZ/2. + pbHalfZ;
    auto* pbMat    = nist->FindOrBuildMaterial("G4_Pb");
    auto* solidPb  = new G4Box("PbBacking", 5.*cm, 5.*cm, pbHalfZ);
    auto* logicPb  = new G4LogicalVolume(solidPb, pbMat, "PbBacking");
    new G4PVPlacement(nullptr, {0,0,pbZpos}, logicPb, "PbBacking", logicWorld, false, 0);

    auto* visPb = new G4VisAttributes(G4Colour(0.5,0.5,0.5,0.6));
    visPb->SetForceSolid(true);
    logicPb->SetVisAttributes(visPb);

    // Superficie óptica centellador→SiPM: dielectric_dielectric polished.
    // Esto ANULA la envoltura de Teflon en esta cara concreta (border > skin).
    // Con n_Si = n_BGO la reflexión de Fresnel es 0 %: todos los fotones entran al SiPM.
    // El SD aplica la PDE para decidir cuáles se "detectan".
    {
        auto* ss = new G4OpticalSurface("ScintToSiPM");
        ss->SetType(dielectric_dielectric);
        ss->SetModel(unified);
        ss->SetFinish(polished);
        // No hace falta tabla de propiedades: la interfaz polished usa
        // ecuaciones de Fresnel basadas en los índices de refracción de los materiales.
        new G4LogicalBorderSurface("Scint_SiPM",physScint,physSiPM,ss);
    }

    // Región con cuts de producción ajustados al tamaño del cristal
    auto* region = new G4Region("ScintRegion");
    auto* cuts   = new G4ProductionCuts();
    cuts->SetProductionCut(0.01*mm, G4ProductionCuts::GetIndex("gamma"));
    cuts->SetProductionCut(0.01*mm, G4ProductionCuts::GetIndex("e-"));
    cuts->SetProductionCut(0.01*mm, G4ProductionCuts::GetIndex("e+"));
    region->AddRootLogicalVolume(fLogicScint);
    region->SetProductionCuts(cuts);

    // Orden debe coincidir con el enum: PLASTIC=0, BGO=1, CSI=2, LYSO=3
    const char* names[] = {"PLASTIC","BGO","CsI:Tl","LYSO:Ce"};
    G4cout << "\n[Detector] Centellador: " << names[(int)fScintType]
           << "  " << scintX/mm << "x" << scintY/mm << "x" << scintZ/mm
           << " mm\n" << G4endl;

    return physWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    auto* sdm = G4SDManager::GetSDMpointer();

    auto* scintSD = new ScintSD("ScintSD");
    sdm->AddNewDetector(scintSD);
    if (fLogicScint) fLogicScint->SetSensitiveDetector(scintSD);

    auto* sipmSD = new OpticalSiPM_SD("SiPM_SD");
    sdm->AddNewDetector(sipmSD);
    if (fLogicSiPM) fLogicSiPM->SetSensitiveDetector(sipmSD);
}
