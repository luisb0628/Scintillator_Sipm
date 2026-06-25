#include "OpticalSiPM_SD.hh"
#include "ScintSD.hh"

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
    fPE_alpha = fPE_Li7 = fPE_gamma = fPE_electron = 0;
    fPE_neutron = fPE_proton = fPE_other = 0;
}

// ---------------------------------------------------------------------------
// ProcessHits — fotón óptico que entra al volumen del SiPM
// ---------------------------------------------------------------------------
G4bool OpticalSiPM_SD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    auto* track = step->GetTrack();

    if (track->GetDefinition() != G4OpticalPhoton::OpticalPhotonDefinition())
        return false;

    if (step->GetPreStepPoint()->GetStepStatus() != fGeomBoundary)
        return false;

    track->SetTrackStatus(fStopAndKill);

    if (G4UniformRand() > fPDE)
        return true;

    ++fPhotonCount;

    // Subir el árbol de parentesco hasta la primaria original.
    // Garantiza que el gamma de 478 keV (primaria) quede en nPE_gamma aunque
    // su energía haya sido depositada por el electrón Compton secundario.
    const G4String prim = ScintSD::GetPrimaryForTrack(track->GetParentID());

    if      (prim == "alpha")    ++fPE_alpha;
    else if (prim == "Li7")      ++fPE_Li7;
    else if (prim == "gamma")    ++fPE_gamma;
    else if (prim == "electron") ++fPE_electron;
    else if (prim == "neutron")  ++fPE_neutron;
    else if (prim == "proton")   ++fPE_proton;
    else                          ++fPE_other;

    auto* am  = G4AnalysisManager::Instance();
    G4int evID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // Ntuple 1 — SiPMData: fotón individual, solo primeros 10 000 eventos
    // Útil para análisis de forma de pulso y espectro óptico
    if (evID < 10000) {
        am->FillNtupleIColumn(1, 0, evID);
        am->FillNtupleSColumn(1, 1, prim);
        am->FillNtupleDColumn(1, 2, track->GetGlobalTime() / ns);
        am->FillNtupleDColumn(1, 3, track->GetKineticEnergy() / eV);
        am->AddNtupleRow(1);
    }

    return true;
}

void OpticalSiPM_SD::EndOfEvent(G4HCofThisEvent*)
{
    auto* am      = G4AnalysisManager::Instance();
    G4int eventID = G4RunManager::GetRunManager()
                        ->GetCurrentEvent()->GetEventID();

    // Ntuple 2 — SiPMSummary: todos los eventos, PE total y por primaria origen
    am->FillNtupleIColumn(2, 0, eventID);
    am->FillNtupleIColumn(2, 1, fPhotonCount);
    am->FillNtupleIColumn(2, 2, fPE_alpha);
    am->FillNtupleIColumn(2, 3, fPE_Li7);
    am->FillNtupleIColumn(2, 4, fPE_gamma);
    am->FillNtupleIColumn(2, 5, fPE_electron);
    am->FillNtupleIColumn(2, 6, fPE_neutron);
    am->FillNtupleIColumn(2, 7, fPE_proton);
    am->FillNtupleIColumn(2, 8, fPE_other);
    am->AddNtupleRow(2);
}
