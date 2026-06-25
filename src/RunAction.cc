#include "RunAction.hh"
#include "EventAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "G4ios.hh"
#include <iomanip>

RunAction::RunAction() : G4UserRunAction() {}
RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run*)
{
    auto* am = G4AnalysisManager::Instance();
    am->SetVerboseLevel(0);
    am->SetNtupleMerging(true);
    fTimer.Start();
    // Resetear barra de progreso (necesario si se lanza más de un /run/beamOn)
    if (G4Threading::IsMasterThread())
        EventAction::ResetProgress();

    am->OpenFile("output.root");

    // ================================================================
    // NTUPLE 0 — ScintEvent: un row por evento
    //   TotalEdep y nOptPhotons son verificación de consistencia:
    //   TotalEdep [keV] × yield [ph/MeV] × eficiencia_óptica ≈ nOptPhotons ≈ nPE_total / PDE
    // ================================================================
    am->CreateNtuple("ScintEvent", "Energia depositada y fotones opticos por evento");
    am->CreateNtupleIColumn("EventID");           // 0
    am->CreateNtupleDColumn("TotalEdep_keV");     // 1
    am->CreateNtupleIColumn("nOptPhotons");       // 2
    am->FinishNtuple();                           // ID=0

    // ================================================================
    // NTUPLE 1 — SiPMData: fotones DETECTADOS por el SiPM
    //   Solo los primeros 10 000 eventos — para análisis de forma de pulso (PSD)
    //   y verificación del espectro óptico del centellador.
    //   PrimaryParticle: categoría de la primaria original que causó el fotón
    //   ("alpha","Li7","gamma","electron","neutron","proton","other")
    // ================================================================
    am->CreateNtuple("SiPMData", "Fotones opticos detectados en SiPM (primeros 10000 eventos)");
    am->CreateNtupleIColumn("EventID");           // 0
    am->CreateNtupleSColumn("PrimaryParticle");   // 1
    am->CreateNtupleDColumn("time_ns");           // 2
    am->CreateNtupleDColumn("energy_eV");         // 3
    am->FinishNtuple();                           // ID=1

    // ================================================================
    // NTUPLE 2 — SiPMSummary: señal del SiPM, todos los eventos
    //   nPE_total  = fotoelectrones totales detectados por el SiPM (PDE=30%)
    //   nPE_*      = fracción atribuida a cada primaria original
    //
    //   Nota importante de interpretación:
    //   - Un gamma primario (ej. 478 keV) → crea electrón Compton
    //     → ese electrón centellea → aparece en nPE_gamma (no en nPE_electron)
    //   - nPE_electron: betas (Sr-90), Compton de un electrón PRIMARIO disparado
    //     por el gun (calibración Cs-137/Co-60 solo en modo beta, etc.)
    //   - La diferencia alpha/Li7 vs gamma es la base del análisis de señal de
    //     captura B-10
    // ================================================================
    am->CreateNtuple("SiPMSummary", "Fotoelectrones por evento y por primaria origen");
    am->CreateNtupleIColumn("EventID");           // 0
    am->CreateNtupleIColumn("nPE_total");         // 1
    am->CreateNtupleIColumn("nPE_alpha");         // 2
    am->CreateNtupleIColumn("nPE_Li7");           // 3
    am->CreateNtupleIColumn("nPE_gamma");         // 4
    am->CreateNtupleIColumn("nPE_electron");      // 5
    am->CreateNtupleIColumn("nPE_neutron");       // 6
    am->CreateNtupleIColumn("nPE_proton");        // 7
    am->CreateNtupleIColumn("nPE_other");         // 8
    am->FinishNtuple();                           // ID=2

    G4cout << "[RunAction] Output: output.root  (3 ntuples: ScintEvent, SiPMData, SiPMSummary)\n";
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    auto* am = G4AnalysisManager::Instance();
    am->Write();
    am->CloseFile();

    fTimer.Stop();

    G4double real = fTimer.GetRealElapsed();
    G4int h = (G4int)(real / 3600);
    G4int m = (G4int)(real / 60) % 60;
    G4int s = (G4int)(real) % 60;

    G4cout << "\n=== Simulacion terminada ===\n";
    G4cout << "Eventos: " << run->GetNumberOfEvent() << "\n";
    G4cout << "Tiempo total: " << h << "h " << m << "m " << s << "s"
           << "  (" << std::fixed << std::setprecision(1) << real << " s)\n";
    G4cout << "Archivo: output.root\n";
    G4cout << "============================\n";
}
