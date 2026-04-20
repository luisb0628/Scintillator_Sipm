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
{}

ScintSD::~ScintSD() {}

// ─── Initialize por evento ────────────────────────────────────────────────────
void ScintSD::Initialize(G4HCofThisEvent*)
{
    fEdepTotal = fEdepAlpha = fEdepLi7 = fEdepGamma = 0.;
    fNOptPhotons = 0;
    fEdepPerTrack.clear();
}

// ─── ProcessHits ─────────────────────────────────────────────────────────────
G4bool ScintSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    auto* track  = step->GetTrack();
    auto* pre    = step->GetPreStepPoint();
    auto* am     = G4AnalysisManager::Instance();

    const G4String pname = track->GetDefinition()->GetParticleName();

    // ── Contar fotones ópticos generados en este step ──────────────────────────
    for (auto* sec : *step->GetSecondaryInCurrentStep()) {
        if (sec->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
            ++fNOptPhotons;
            // Ntuple 4 (OpticalGen) solo se llena para el primer evento
            // como diagnóstico — evita archivos de GB con 40 M filas.
            if (fNOptPhotons <= 10000 &&
                G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID() == 0) {
                const G4VProcess* proc = sec->GetCreatorProcess();
                G4String procName = proc ? proc->GetProcessName() : "None";
                G4int evID = 0;
                am->FillNtupleIColumn(4, 0, evID);
                am->FillNtupleIColumn(4, 1, track->GetTrackID());
                am->FillNtupleSColumn(4, 2, procName);
                am->FillNtupleDColumn(4, 3, sec->GetKineticEnergy() / eV);
                am->FillNtupleDColumn(4, 4, sec->GetPosition().x() / mm);
                am->FillNtupleDColumn(4, 5, sec->GetPosition().y() / mm);
                am->FillNtupleDColumn(4, 6, sec->GetPosition().z() / mm);
                am->FillNtupleDColumn(4, 7, sec->GetGlobalTime() / ns);
                am->FillNtupleSColumn(4, 8, pname);
                am->AddNtupleRow(4);
            }
        }
    }

    // ── Fotones ópticos no depositan energía — saltar el resto ─────────────────
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
        return false;

    G4double edep = step->GetTotalEnergyDeposit();
    if (edep <= 0.) return false;

    // ── Acumular Edep por tipo de partícula ────────────────────────────────────
    fEdepTotal += edep;
    fEdepPerTrack[track->GetTrackID()] += edep;

    // En Geant4: alpha → "alpha", Li-7 → "Li7[0.0]" (incluye energía de excitación)
    if (pname == "alpha")
        fEdepAlpha += edep;
    else if (pname.substr(0,3) == "Li7")
        fEdepLi7 += edep;
    else  // gamma, e-, e+ → todos van a "gamma" (misma respuesta en centellador)
        fEdepGamma += edep;

    // ── Ntuple 0 — ScintData (detalle por step, solo primeros 100 eventos) ──────
    G4int evID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
    if (evID < 100) {
        const G4VProcess* creator = track->GetCreatorProcess();
        am->FillNtupleIColumn(0, 0, evID);
        am->FillNtupleIColumn(0, 1, track->GetTrackID());
        am->FillNtupleSColumn(0, 2, pname);
        am->FillNtupleDColumn(0, 3, pre->GetKineticEnergy() / MeV);
        am->FillNtupleDColumn(0, 4, edep / MeV);
        am->FillNtupleDColumn(0, 5, pre->GetPosition().x() / mm);
        am->FillNtupleDColumn(0, 6, pre->GetPosition().y() / mm);
        am->FillNtupleDColumn(0, 7, pre->GetPosition().z() / mm);
        am->FillNtupleSColumn(0, 8, creator ? creator->GetProcessName() : "primary");
        am->AddNtupleRow(0);
    }

    return true;
}

// ─── EndOfEvent ───────────────────────────────────────────────────────────────
void ScintSD::EndOfEvent(G4HCofThisEvent*)
{
    auto* am  = G4AnalysisManager::Instance();
    G4int evID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // ── Histogramas (visibles directamente en TBrowser) ──────────────────────
    if (fEdepTotal > 0.) {
        am->FillH1(0, fEdepTotal / keV);           // Edep total
        if (fEdepAlpha > 0.) am->FillH1(2, fEdepAlpha / keV);
        if (fEdepLi7   > 0.) am->FillH1(3, fEdepLi7   / keV);
        if (fEdepGamma > 0.) am->FillH1(4, fEdepGamma / keV);
    }
    if (fNOptPhotons > 0)
        am->FillH1(5, G4double(fNOptPhotons));     // fotones ópticos generados

    // ── Ntuple 1 — ScintEvent (resumen por evento) ────────────────────────────
    am->FillNtupleIColumn(1, 0, evID);
    am->FillNtupleDColumn(1, 1, fEdepTotal / keV);
    am->FillNtupleDColumn(1, 2, fEdepAlpha / keV);
    am->FillNtupleDColumn(1, 3, fEdepLi7   / keV);
    am->FillNtupleDColumn(1, 4, fEdepGamma / keV);
    am->FillNtupleIColumn(1, 5, fNOptPhotons);
    am->AddNtupleRow(1);
}
