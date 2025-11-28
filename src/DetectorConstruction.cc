// DetectorConstruction.cc
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
#include "G4Isotope.hh"
#include "G4Element.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4ProductionCuts.hh"
#include "G4Region.hh"
#include "G4SDManager.hh"
#include "G4UserLimits.hh"

#include "G4LogicalBorderSurface.hh"
#include "G4OpticalSurface.hh"
#include "G4MaterialPropertiesTable.hh"

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fScintType(ScintType::PLASTIC)   // Cambia aquí el tipo de centellador
{}

DetectorConstruction::~DetectorConstruction() = default;

//
// -------------------------------------------
// MATERIALES DEL CENTELLADOR + PROPIEDADES ÓPTICAS
// -------------------------------------------
//
G4Material* DetectorConstruction::CreateScintillatorMaterial()
{
    auto nist = G4NistManager::Instance();
    G4Material* base = nullptr;

    switch (fScintType)
    {
        case ScintType::PLASTIC:
            // Aproximación a 
            base = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
            break;

        case ScintType::BGO:
            base = nist->FindOrBuildMaterial("G4_BGO");
            break;

        case ScintType::CSI:
            // CsI (puedes suponer CsI(Tl) para las propiedades ópticas)
            base = nist->FindOrBuildMaterial("G4_CESIUM_IODIDE");
            break;

        case ScintType::LYSO:
            base = nist->FindOrBuildMaterial("G4_LSO");
            break;

        default:
            base = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
            break;
    }

    DefineOpticalProperties(base);
    return base;
}

void DetectorConstruction::DefineOpticalProperties(G4Material* scintMat)
{
    // Espectro de energía (simplificado, 2 puntos)
    constexpr G4int n = 2;
    G4double photonE[n] = { 2.0*eV, 3.5*eV };

    auto mpt = new G4MaterialPropertiesTable();

    // ============================================================
    // PLÁSTICO tipo EJ-200 / BC-408
    // ============================================================
    if (fScintType == ScintType::PLASTIC)
    {
        G4double rindex[n] = { 1.58, 1.58 };
        G4double abslen[n] = { 380*cm, 380*cm }; // longitud de absorción típica
        // Espectro FAST simplificado (pico ~425 nm)
        G4double fast[n]   = { 1.0, 0.1 };

        mpt->AddProperty("RINDEX",      photonE, rindex,   n);
        mpt->AddProperty("ABSLENGTH",   photonE, abslen,   n);
        mpt->AddProperty("FASTCOMPONENT", photonE, fast,   n, true);

        // Constantes de centelleo (valores típicos)
        mpt->AddConstProperty("SCINTILLATIONYIELD", 10000./MeV, true);
        mpt->AddConstProperty("RESOLUTIONSCALE",    1.0,        true);
        mpt->AddConstProperty("FASTTIMECONSTANT",   2.1*ns,     true);
        mpt->AddConstProperty("YIELDRATIO",         1.0,        true);

        // Birks para plásticos
        if (scintMat->GetIonisation())
            scintMat->GetIonisation()->SetBirksConstant(0.126*mm/MeV);
    }
    // ============================================================
    // BGO
    // ============================================================
    else if (fScintType == ScintType::BGO)
    {
        G4double rindex[n] = { 2.15, 2.15 };
        G4double abslen[n] = { 55*cm, 55*cm };
        // Emisión ~480 nm
        G4double fast[n]   = { 1.0, 0.1 };

        mpt->AddProperty("RINDEX",      photonE, rindex,   n);
        mpt->AddProperty("ABSLENGTH",   photonE, abslen,   n);
        mpt->AddProperty("FASTCOMPONENT", photonE, fast,   n, true);

        mpt->AddConstProperty("SCINTILLATIONYIELD", 8500./MeV, true);
        mpt->AddConstProperty("RESOLUTIONSCALE",    1.0,       true);
        mpt->AddConstProperty("FASTTIMECONSTANT",   300.*ns,   true);
        mpt->AddConstProperty("YIELDRATIO",         1.0,       true);
    }
    // ============================================================
    // LYSO:Ce
    // ============================================================
    else if (fScintType == ScintType::LYSO)
    {
        G4double rindex[n] = { 1.82, 1.82 };
        G4double abslen[n] = { 40*cm, 40*cm };
        // Emisión ~420–430 nm
        G4double fast[n]   = { 1.0, 0.15 };

        mpt->AddProperty("RINDEX",      photonE, rindex,   n);
        mpt->AddProperty("ABSLENGTH",   photonE, abslen,   n);
        mpt->AddProperty("FASTCOMPONENT", photonE, fast,   n, true);

        mpt->AddConstProperty("SCINTILLATIONYIELD", 32000./MeV, true);
        mpt->AddConstProperty("RESOLUTIONSCALE",    1.0,        true);
        mpt->AddConstProperty("FASTTIMECONSTANT",   40.*ns,     true);
        mpt->AddConstProperty("YIELDRATIO",         1.0,        true);
    }
    // ============================================================
    // CsI:Tl
    // ============================================================
    else if (fScintType == ScintType::CSI)
    {
        G4double rindex[n] = { 1.80, 1.80 };
        G4double abslen[n] = { 2.0*m, 2.0*m };
        // Emisión ~540–550 nm
        G4double fast[n]   = { 1.0, 0.2 };

        mpt->AddProperty("RINDEX",      photonE, rindex,   n);
        mpt->AddProperty("ABSLENGTH",   photonE, abslen,   n);
        mpt->AddProperty("FASTCOMPONENT", photonE, fast,   n, true);

        mpt->AddConstProperty("SCINTILLATIONYIELD", 54000./MeV, true);
        mpt->AddConstProperty("RESOLUTIONSCALE",    1.0,        true);
        mpt->AddConstProperty("FASTTIMECONSTANT",   1000.*ns,   true);
        mpt->AddConstProperty("YIELDRATIO",         1.0,        true);
    }

    scintMat->SetMaterialPropertiesTable(mpt);
}

//
// -------------------------------------------
// CONSTRUCCIÓN DEL DETECTOR
// -------------------------------------------
//
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto nist = G4NistManager::Instance();

    // ============================
    // WORLD
    // ============================
    auto worldMat = nist->FindOrBuildMaterial("G4_AIR");
    auto solidWorld = new G4Box("World", 20*cm, 20*cm, 20*cm);
    auto logicWorld = new G4LogicalVolume(solidWorld, worldMat, "World");
    auto physWorld  = new G4PVPlacement(nullptr, {}, logicWorld, "World", 0, false, 0);

    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
    fPhysWorld = physWorld;

    // Propiedades ópticas del aire (para que los fotones puedan viajar)
    {
        const G4int n = 2;
        G4double photonE[n] = { 2.0*eV, 3.5*eV };
        G4double rindex[n]  = { 1.0, 1.0 };

        auto mptWorld = new G4MaterialPropertiesTable();
        mptWorld->AddProperty("RINDEX", photonE, rindex, n);
        worldMat->SetMaterialPropertiesTable(mptWorld);
    }

    // ============================
    // GRAPHENE + BORO-10
    // ============================
    auto B10 = new G4Isotope("B10", 5, 10, 10.0*g/mole);
    auto B11 = new G4Isotope("B11", 5, 11, 11.0*g/mole);

    auto elB_enriched = new G4Element("BoronEnriched", "B_enr", 2);
    elB_enriched->AddIsotope(B10, 100*perCent);
    elB_enriched->AddIsotope(B11, 0);

    auto graphene = new G4Material("graphene", 2.2*g/cm3, 2);
    graphene->AddElement(nist->FindOrBuildElement("C"), 0.85);
    graphene->AddElement(elB_enriched, 0.15);

    G4double graphThickness = 50*nm;
    G4double graphHalfZ = graphThickness/2.0;

    auto solidGraph = new G4Box("graphene", 1*cm, 1*cm, graphHalfZ);
    auto logicGraph = new G4LogicalVolume(solidGraph, graphene, "graphene");

    auto physGraph = new G4PVPlacement(nullptr, {0,0,0}, logicGraph,
                                       "graphene", logicWorld, false, 0);

    auto visGraph = new G4VisAttributes(G4Colour(1, 1, 1, 1));
    visGraph->SetForceSolid(true);
    logicGraph->SetVisAttributes(visGraph);

    // ============================
    // KAPTON
    // ============================
    G4double kapThickness = 1*um;
    G4double kapHalfZ = kapThickness/2.0;

    auto kaptonMat = nist->FindOrBuildMaterial("G4_KAPTON");
    G4double kapZ = graphHalfZ + kapHalfZ;

    auto solidKap = new G4Box("kapton", 1.5*cm, 1.5*cm, kapHalfZ);
    auto logicKap = new G4LogicalVolume(solidKap, kaptonMat, "kapton");

    auto physKap = new G4PVPlacement(nullptr, {0,0,kapZ}, logicKap,
                                     "kapton", logicWorld, false, 0);

    auto visKap = new G4VisAttributes(G4Colour(1.0, 0.7, 0.3, 0.8));
    visKap->SetForceSolid(true);
    logicKap->SetVisAttributes(visKap);

    // ============================
    // CENTELLADOR (dimensiones según tipo)
    // ============================
    G4double scintX, scintY, scintZ, scintHalfZ;

    if (fScintType == ScintType::PLASTIC)
    {
        //
        scintX = 4.7*cm;
        scintY = 4.7*cm;
        scintZ = 1.0*cm;
    }
    else
    {
        // Cristales: BGO, LYSO, CsI:Tl
        scintX = 7.0*mm;
        scintY = 7.0*mm;
        scintZ = 16.0*mm;
    }

    scintHalfZ = scintZ/2.0;

    // Centro del centellador en Z
    G4double scintZpos = graphHalfZ + scintHalfZ;

    auto scintMat = CreateScintillatorMaterial();

    auto solidScint = new G4Box("Scintillator",
                                scintX/2.0, scintY/2.0, scintHalfZ);
    auto logicScint = new G4LogicalVolume(solidScint, scintMat, "Scintillator");

    auto physScint = new G4PVPlacement(nullptr, {0,0,scintZpos}, logicScint,
                                       "Scintillator", logicWorld, false, 0);

    auto visScint = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0, 0.3));
    visScint->SetForceSolid(true);
    logicScint->SetVisAttributes(visScint);

    // ============================
    // SiPM
    // ============================
    G4double sipmX = 4.7*cm;
    G4double sipmY = 4.7*cm; //5mm
    G4double sipmZ = 1*mm;

    auto solidSiPM = new G4Box("SiPM", sipmX/2, sipmY/2, sipmZ/2);
    auto sipmMat = nist->FindOrBuildMaterial("G4_Si");

    // Propiedades ópticas del Silicio
    {
        const G4int nSi = 2;
        G4double eSi[nSi]   = { 2.0*eV, 3.5*eV };
        G4double rSi[nSi]   = { 3.5, 3.5 };         // índice alto del Si
        G4double absSi[nSi] = { 0.001*mm, 0.001*mm }; // muy absorbente

        auto mptSi = new G4MaterialPropertiesTable();
        mptSi->AddProperty("RINDEX",    eSi, rSi,   nSi);
        mptSi->AddProperty("ABSLENGTH", eSi, absSi, nSi);
        sipmMat->SetMaterialPropertiesTable(mptSi);
    }

    auto logicSiPM = new G4LogicalVolume(solidSiPM, sipmMat, "SiPM");

    // Posición del SiPM: tocando el centellador (sin gap)
    G4double sipmZpos = scintZpos + scintHalfZ + sipmZ/2.0;

    auto physSiPM = new G4PVPlacement(nullptr, {0,0,sipmZpos}, logicSiPM,
                                      "SiPM", logicWorld, false, 0);

    auto visSiPM = new G4VisAttributes(G4Colour(1.0, 0.0, 1.0, 1.0));
    visSiPM->SetForceSolid(true);
    logicSiPM->SetVisAttributes(visSiPM);

    // ============================
    // Superficie óptica Scintillator–SiPM
    // ============================
    auto optSurf = new G4OpticalSurface("ScintToSiPM");
    optSurf->SetType(dielectric_dielectric);
    optSurf->SetModel(unified);
    optSurf->SetFinish(polished);

    // Eficiencia cuántica (PDE simplificada)
    auto mptSurf = new G4MaterialPropertiesTable();
    const G4int num = 2;
    G4double pp[num] = {2.0*eV, 3.5*eV};
    G4double efficiency[num] = {1.0, 1.0}; 

    mptSurf->AddProperty("EFFICIENCY", pp, efficiency, num);
    optSurf->SetMaterialPropertiesTable(mptSurf);

    new G4LogicalBorderSurface("Scint_SiPM_Surface",
                               physScint, physSiPM, optSurf);

    // ============================
    // Production cuts & Regions
    // ============================
    auto cuts = new G4ProductionCuts();
    cuts->SetProductionCut(0.01*mm, G4ProductionCuts::GetIndex("gamma"));
    cuts->SetProductionCut(0.01*mm, G4ProductionCuts::GetIndex("neutron"));

    auto region = new G4Region("DetectorRegion");
    region->AddRootLogicalVolume(logicGraph);
    region->AddRootLogicalVolume(logicKap);
    region->AddRootLogicalVolume(logicScint);
    region->SetProductionCuts(cuts);

    return physWorld;
    G4cout << "=== SCINT SELECTED: " << (int)fScintType << G4endl;
}


//
// -------------------------------------------
// SENSITIVE DETECTORS
// -------------------------------------------
//
void DetectorConstruction::ConstructSDandField()
{
    auto sdManager = G4SDManager::GetSDMpointer();

    // SD del centellador
    auto scintSD = new ScintSD("ScintSD");
    sdManager->AddNewDetector(scintSD);

    auto lvScint = G4LogicalVolumeStore::GetInstance()->GetVolume("Scintillator");
    if (lvScint)
        lvScint->SetSensitiveDetector(scintSD);

    // SD del SiPM
    auto sipmSD = new OpticalSiPM_SD("SiPM_SD");
    sdManager->AddNewDetector(sipmSD);

    auto lvSiPM = G4LogicalVolumeStore::GetInstance()->GetVolume("SiPM");
    if (lvSiPM)
        lvSiPM->SetSensitiveDetector(sipmSD);
}
