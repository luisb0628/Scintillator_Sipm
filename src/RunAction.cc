#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include <iomanip>

RunAction::RunAction() : G4UserRunAction() {}
RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run*)
{
    auto* am = G4AnalysisManager::Instance();
    am->SetVerboseLevel(0);
    fTimer.Start();
    am->OpenFile("output.root");

    // ================================================================
    // HISTOGRAMAS TH1 — visibles directamente en TBrowser de ROOT
    // ================================================================
    // H1 ID=0  Espectro de energía depositada total (lo que "absorbe" el cristal)
    am->CreateH1("Edep_keV",
                 "Energia depositada total en centellador;Edep (keV);Cuentas",
                 500, 0., 10000.);                               // ID=0

    // H1 ID=1  Espectro de fotoelectrones (señal real del SiPM)
    am->CreateH1("nPhotoelectrons",
                 "Fotoelectrones detectados por SiPM;N_{PE};Cuentas",
                 500, 0., 50000.);                               // ID=1

    // H1 ID=2  Edep solo de alfas (quenching de Birks reduce su señal)
    am->CreateH1("Edep_alpha_keV",
                 "Edep de particulas #alpha;Edep (keV);Cuentas",
                 300, 0., 6000.);                                // ID=2

    // H1 ID=3  Edep solo de Li-7
    am->CreateH1("Edep_Li7_keV",
                 "Edep de iones Li-7;Edep (keV);Cuentas",
                 300, 0., 3000.);                                // ID=3

    // H1 ID=4  Edep solo de gammas (y electrones secundarios)
    am->CreateH1("Edep_gamma_keV",
                 "Edep de gammas / e^{#pm};Edep (keV);Cuentas",
                 300, 0., 3000.);                                // ID=4

    // H1 ID=5  Fotones ópticos GENERADOS por evento (antes de PDE)
    am->CreateH1("nOptPhotons_gen",
                 "Fotones opticos generados por evento;N_{fotones};Cuentas",
                 500, 0., 100000.);                              // ID=5

    // ================================================================
    // NTUPLE 0 — ScintData (Edep por step)
    // ================================================================
    am->CreateNtuple("ScintData", "Edep por step en el centellador");
    am->CreateNtupleIColumn("EventID");           // 0
    am->CreateNtupleIColumn("TrackID");           // 1
    am->CreateNtupleSColumn("Particle");          // 2
    am->CreateNtupleDColumn("KineticEnergy_MeV"); // 3
    am->CreateNtupleDColumn("Edep_MeV");          // 4
    am->CreateNtupleDColumn("X_mm");              // 5
    am->CreateNtupleDColumn("Y_mm");              // 6
    am->CreateNtupleDColumn("Z_mm");              // 7
    am->CreateNtupleSColumn("Creator");           // 8
    am->FinishNtuple();                           // ID=0

    // ================================================================
    // NTUPLE 1 — ScintEvent (Edep total + por tipo de partícula por evento)
    // ================================================================
    am->CreateNtuple("ScintEvent", "Energia total por evento (histograma principal)");
    am->CreateNtupleIColumn("EventID");           // 0
    am->CreateNtupleDColumn("TotalEdep_keV");     // 1  (cambiado a keV para facilidad)
    am->CreateNtupleDColumn("Edep_alpha_keV");    // 2
    am->CreateNtupleDColumn("Edep_Li7_keV");      // 3
    am->CreateNtupleDColumn("Edep_gamma_keV");    // 4
    am->CreateNtupleIColumn("nOptPhotons");       // 5  fotones ópticos generados
    am->FinishNtuple();                           // ID=1

    // ================================================================
    // NTUPLE 2 — SiPMData (fotones detectados individuales)
    // ================================================================
    am->CreateNtuple("SiPMData", "Fotones opticos detectados en SiPM");
    am->CreateNtupleDColumn("time_ns");           // 0
    am->CreateNtupleDColumn("energy_eV");         // 1
    am->CreateNtupleDColumn("x_mm");              // 2
    am->CreateNtupleDColumn("y_mm");              // 3
    am->CreateNtupleDColumn("z_mm");              // 4
    am->FinishNtuple();                           // ID=2

    // ================================================================
    // NTUPLE 3 — SiPMSummary (fotoelectrones totales por evento)
    // ================================================================
    am->CreateNtuple("SiPMSummary", "Fotoelectrones por evento (espectro del SiPM)");
    am->CreateNtupleIColumn("EventID");           // 0
    am->CreateNtupleIColumn("nPhotoelectrons");   // 1
    am->FinishNtuple();                           // ID=3

    // ================================================================
    // NTUPLE 4 — OpticalGen (fotones ópticos generados, detalles)
    // ================================================================
    am->CreateNtuple("OpticalGen", "Fotones opticos generados en centellador");
    am->CreateNtupleIColumn("EventID");           // 0
    am->CreateNtupleIColumn("ParentTrackID");     // 1
    am->CreateNtupleSColumn("CreatorProcess");    // 2
    am->CreateNtupleDColumn("energy_eV");         // 3
    am->CreateNtupleDColumn("x_mm");              // 4
    am->CreateNtupleDColumn("y_mm");              // 5
    am->CreateNtupleDColumn("z_mm");              // 6
    am->CreateNtupleDColumn("t_ns");              // 7
    am->CreateNtupleSColumn("ParentParticle");    // 8
    am->FinishNtuple();                           // ID=4

    G4cout << "[RunAction] Output: output.root  (6 histogramas + 5 ntuples)\n";
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

    G4cout << "\n=== Simulación terminada ===\n";
    G4cout << "Eventos: " << run->GetNumberOfEvent() << "\n";
    G4cout << "Tiempo total: " << h << "h " << m << "m " << s << "s"
           << "  (" << std::fixed << std::setprecision(1) << real << " s)\n";
    G4cout << "Archivo: output.root\n";
    G4cout << "Histogramas listos para TBrowser o analysis_scint.py\n";
    G4cout << "============================\n";
}
