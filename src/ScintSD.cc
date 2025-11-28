#include "ScintSD.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpticalPhoton.hh"
#include "G4VProcess.hh"

ScintSD::ScintSD(const G4String& name)
    : G4VSensitiveDetector(name)
{
}

ScintSD::~ScintSD() {}

// =============================================================
// Inicializa datos por evento
// =============================================================
void ScintSD::Initialize(G4HCofThisEvent*)
{
    fEdepPerTrack.clear();
}

// =============================================================
// ProcessHits: registra Edep Y registra fotones ópticos generados
// =============================================================
G4bool ScintSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    auto analysis = G4AnalysisManager::Instance();

    // ------------------------------------------------------------
    // Información básica del step y del TRACK ACTUAL (El "Padre" de los fotones)
    // ------------------------------------------------------------
    const G4Track* track = step->GetTrack();
    auto pre = step->GetPreStepPoint();

    // Obtenemos el nombre de la partícula que está depositando energía o generando luz.
    // Para neutrones+Boro, aquí esperas ver: "Li7", "alpha"
    // Para gammas interactuando, verás: "e-" (electrones secundarios)
    G4String parentName = track->GetDefinition()->GetParticleName();

    G4double edep = step->GetTotalEnergyDeposit();
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
    G4int tid     = track->GetTrackID();

    // ============================================================
    // 1) REGISTRAR DEPÓSITO DE ENERGÍA (NTUPLE 0)
    // ============================================================
    if (edep > 0.)
    {
        fEdepPerTrack[tid] += edep;

        const G4VProcess* creator = track->GetCreatorProcess();

        analysis->FillNtupleIColumn(0, 0, eventID);
        analysis->FillNtupleIColumn(0, 1, tid);
        analysis->FillNtupleSColumn(0, 2, parentName); // Usamos la var parentName
        analysis->FillNtupleDColumn(0, 3, pre->GetKineticEnergy() / MeV);
        analysis->FillNtupleDColumn(0, 4, edep / MeV);
        analysis->FillNtupleDColumn(0, 5, pre->GetPosition().x() / mm);
        analysis->FillNtupleDColumn(0, 6, pre->GetPosition().y() / mm);
        analysis->FillNtupleDColumn(0, 7, pre->GetPosition().z() / mm);
        analysis->FillNtupleSColumn(0, 8,
            creator ? creator->GetProcessName() : "primary");

        analysis->AddNtupleRow(0);
    }

    // ============================================================
    // 2) REGISTRAR FOTONES ÓPTICOS GENERADOS EN ESTE STEP (NTUPLE 4)
    // ============================================================
    auto secondaries = step->GetSecondaryInCurrentStep();

    // Iteramos sobre las partículas creadas en este paso (ej. fotones ópticos)
    for (auto secTrack : *secondaries)
    {
        // Verificamos que sea un fotón óptico
        if (secTrack->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
        {
            const G4VProcess* proc = secTrack->GetCreatorProcess();
            G4String procName = proc ? proc->GetProcessName() : "None";

            analysis->FillNtupleIColumn(4, 0, eventID);                      // EventID
            analysis->FillNtupleIColumn(4, 1, tid);                          // Parent TrackID
            analysis->FillNtupleIColumn(4, 2, secTrack->GetTrackID());       // Photon TrackID
            analysis->FillNtupleSColumn(4, 3, procName);                     // Creator process

            // --- CAMBIO IMPORTANTE: Unidades en eV ---
            // Los fotones ópticos tienen energías de ~2-3 eV. 
            // Si usas MeV, perderás precisión.
            analysis->FillNtupleDColumn(4, 4, secTrack->GetKineticEnergy() / eV);

            analysis->FillNtupleDColumn(4, 5, secTrack->GetPosition().x() / mm);
            analysis->FillNtupleDColumn(4, 6, secTrack->GetPosition().y() / mm);
            analysis->FillNtupleDColumn(4, 7, secTrack->GetPosition().z() / mm);
            analysis->FillNtupleDColumn(4, 8, secTrack->GetGlobalTime() / ns);
            
            // --- NUEVO CAMBIO: Guardar quién generó el fotón ---
            // Esto guardará "Li7", "alpha", o "e-" en la columna 9
            analysis->FillNtupleSColumn(4, 9, parentName);

            analysis->AddNtupleRow(4);
        }
    }

    return true;
}

// =============================================================
// EndOfEvent: energía total del evento (NTUPLE 1)
// =============================================================
void ScintSD::EndOfEvent(G4HCofThisEvent*)
{
    auto analysis = G4AnalysisManager::Instance();
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    G4double totalE = 0.;
    for (auto& kv : fEdepPerTrack)
        totalE += kv.second;

    analysis->FillNtupleIColumn(1, 0, eventID);
    analysis->FillNtupleDColumn(1, 1, totalE / MeV);
    analysis->AddNtupleRow(1);
}