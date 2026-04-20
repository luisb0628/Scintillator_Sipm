#include "OpticalSiPM_SD.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

OpticalSiPM_SD::OpticalSiPM_SD(const G4String& name)
    : G4VSensitiveDetector(name), fPhotonCount(0)
{}

void OpticalSiPM_SD::Initialize(G4HCofThisEvent*)
{
    fPhotonCount = 0;
}

// ---------------------------------------------------------------------------
// ProcessHits — solo se llama cuando un track deposita energía en el SiPM.
// Para fotones ópticos con dielectric_metal la frontera absorbe el fotón y
// llama a este SD. La condición correcta es: ES un fotón óptico y ENTRA
// (fGeomBoundary en el pre-step-point).
// ---------------------------------------------------------------------------
G4bool OpticalSiPM_SD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    auto* track = step->GetTrack();

    // SOLO fotones ópticos (antes estaba invertido — BUG corregido)
    if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition())
        return false;

    // SOLO cuando cruza la frontera de entrada al SiPM (antes estaba invertido — BUG corregido)
    if (step->GetPreStepPoint()->GetStepStatus() != fGeomBoundary)
        return false;

    // Matar el fotón: ya fue absorbido por el silicio
    track->SetTrackStatus(fStopAndKill);

    // Aplicar PDE (Photo-Detection Efficiency del 30 %)
    if (G4UniformRand() > fPDE)
        return true;   // fotón llegó pero no generó señal

    // Fotón detectado → registrar
    ++fPhotonCount;

    const G4ThreeVector& pos  = step->GetPreStepPoint()->GetPosition();
    G4double             time = track->GetGlobalTime();
    G4double             ekin = track->GetKineticEnergy();

    auto* am = G4AnalysisManager::Instance();
    // Ntuple 2 — SiPMData solo para primeros 100 eventos (diagnóstico)
    G4int evID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
    if (evID < 100) {
        am->FillNtupleDColumn(2, 0, time / ns);
        am->FillNtupleDColumn(2, 1, ekin / eV);
        am->FillNtupleDColumn(2, 2, pos.x() / mm);
        am->FillNtupleDColumn(2, 3, pos.y() / mm);
        am->FillNtupleDColumn(2, 4, pos.z() / mm);
        am->AddNtupleRow(2);
    }

    return true;
}

void OpticalSiPM_SD::EndOfEvent(G4HCofThisEvent*)
{
    auto* am      = G4AnalysisManager::Instance();
    G4int eventID = G4RunManager::GetRunManager()
                        ->GetCurrentEvent()->GetEventID();

    // H1[1] — histograma de fotoelectrones (visible en TBrowser)
    if (fPhotonCount > 0)
        am->FillH1(1, G4double(fPhotonCount));

    // Ntuple 3 — SiPMSummary: EventID | nPhotoelectrons
    am->FillNtupleIColumn(3, 0, eventID);
    am->FillNtupleIColumn(3, 1, fPhotonCount);
    am->AddNtupleRow(3);
}
