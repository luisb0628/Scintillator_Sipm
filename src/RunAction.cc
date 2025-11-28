#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4ios.hh"

RunAction::RunAction() : G4UserRunAction() {}
RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run*)
{      
    auto analysisManager = G4AnalysisManager::Instance();

    G4cout << "AnalysisManager: " << analysisManager << G4endl;
    G4cout << "Backend: " << analysisManager->GetType() << G4endl;

    // ============================================================
    // ABRIR ARCHIVO ROOT
    // ============================================================
    analysisManager->OpenFile("output.root");
    G4cout << "Archivo ROOT abierto correctamente.\n";

    // ============================================================
    // NTUPLE 0 – ScintData (Edep por step en el centellador)
    // ============================================================
    analysisManager->CreateNtuple("ScintData", "Energy deposition per step in scintillator");

    analysisManager->CreateNtupleIColumn("EventID");               // 0
    analysisManager->CreateNtupleIColumn("TrackID");               // 1
    analysisManager->CreateNtupleSColumn("Particle");              // 2
    analysisManager->CreateNtupleDColumn("KineticEnergy_MeV");     // 3
    analysisManager->CreateNtupleDColumn("Edep_MeV");              // 4
    analysisManager->CreateNtupleDColumn("X_mm");                  // 5
    analysisManager->CreateNtupleDColumn("Y_mm");                  // 6
    analysisManager->CreateNtupleDColumn("Z_mm");                  // 7
    analysisManager->CreateNtupleSColumn("Creator");               // 8

    analysisManager->FinishNtuple();                               // ID = 0

    // ============================================================
    // NTUPLE 1 – ScintEvent (Edep total por evento)
    // ============================================================
    analysisManager->CreateNtuple("ScintEvent", "Total energy per event");

    analysisManager->CreateNtupleIColumn("EventID");               // 0
    analysisManager->CreateNtupleDColumn("TotalEdep_MeV");         // 1

    analysisManager->FinishNtuple();                               // ID = 1

    // ============================================================
    // NTUPLE 2 – SiPMData (Fotones ópticos detectados)
    // ============================================================
    analysisManager->CreateNtuple("SiPMData", "Optical photons detected at SiPM");

    analysisManager->CreateNtupleDColumn("time_ns");               // 0
    analysisManager->CreateNtupleDColumn("energy_eV");             // 1
    analysisManager->CreateNtupleDColumn("x_mm");                  // 2
    analysisManager->CreateNtupleDColumn("y_mm");                  // 3
    analysisManager->CreateNtupleDColumn("z_mm");                  // 4

    analysisManager->FinishNtuple();                               // ID = 2

    // ============================================================
    // NTUPLE 3 – SiPMSummary (Conteo de fotones por evento)
    // ============================================================
    analysisManager->CreateNtuple("SiPMSummary", "Total photons detected per event");

    analysisManager->CreateNtupleIColumn("EventID");               // 0
    analysisManager->CreateNtupleIColumn("nPhotons");              // 1

    analysisManager->FinishNtuple();                               // ID = 3


    // ============================================================
    // NTUPLE 4 – OpticalGen (fotones ópticos GENERADOS)
    // ============================================================
    analysisManager->CreateNtuple("OpticalGen", "Optical photons generated in scintillator");

    analysisManager->CreateNtupleIColumn("EventID");               // 0
    analysisManager->CreateNtupleIColumn("ParentTrackID");         // 1
    analysisManager->CreateNtupleIColumn("PhotonTrackID");         // 2
    analysisManager->CreateNtupleSColumn("CreatorProcess");        // 3
    
    // CAMBIO 1: Etiqueta cambiada a eV para reflejar el cambio en ScintSD
    analysisManager->CreateNtupleDColumn("energy_eV");             // 4
    
    analysisManager->CreateNtupleDColumn("x_mm");                  // 5
    analysisManager->CreateNtupleDColumn("y_mm");                  // 6
    analysisManager->CreateNtupleDColumn("z_mm");                  // 7
    analysisManager->CreateNtupleDColumn("t_ns");                  // 8
    
    // CAMBIO 2: Nueva columna para identificar si vino de Li7, alpha o e-
    analysisManager->CreateNtupleSColumn("ParentName");            // 9

    analysisManager->FinishNtuple();                               // ID = 4

    // Fin de creación
    G4cout << ">>> Todos los NTUPLES se crearon correctamente.\n";
}


void RunAction::EndOfRunAction(const G4Run* run)
{
    auto analysisManager = G4AnalysisManager::Instance();

    // --- Guardar archivo ROOT ---
    analysisManager->Write();
    analysisManager->CloseFile();

    G4cout << "\n=========== ESTADÍSTICAS DEL RUN ===========\n";
    G4cout << "Eventos procesados: " << run->GetNumberOfEvent() << G4endl;
    G4cout << "Archivo ROOT guardado como: output.root\n";
    G4cout << "=============================================\n";
}