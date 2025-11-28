#include "OpticalSiPM_SD.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4RandomTools.hh"

OpticalSiPM_SD::OpticalSiPM_SD(const G4String& name)
: G4VSensitiveDetector(name),
  fPhotonCount(0)
{}

void OpticalSiPM_SD::Initialize(G4HCofThisEvent*)
{
    fPhotonCount = 0;   // contador limpio por evento
}

G4bool OpticalSiPM_SD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    auto track = step->GetTrack();

    // Solo optical photons
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
        return false;

    // Solo cuando entra al volumen SiPM
    if (step->GetPreStepPoint()->GetStepStatus() == fGeomBoundary)
        return false;

    // --- PDE: probabilidad realista del SiPM ---
    if (G4UniformRand() > fPDE)
    {
        track->SetTrackStatus(fStopAndKill);
        return true;  // fotón llegó, pero no fue detectado
    }

    // --- Si fue detectado → incrementar contador ---
    fPhotonCount++;

    // Guardar algunos datos del fotón
    auto post = step->GetPostStepPoint();
    auto pos = post->GetPosition();
    auto time = track->GetGlobalTime();

    auto analysis = G4AnalysisManager::Instance();
    analysis->FillNtupleDColumn(2, 0, time/ns);
    analysis->FillNtupleDColumn(2, 1, pos.x()/mm);
    analysis->FillNtupleDColumn(2, 2, pos.y()/mm);
    analysis->FillNtupleDColumn(2, 3, pos.z()/mm);
    analysis->AddNtupleRow(2);

    track->SetTrackStatus(fStopAndKill);

    return true;
}

void OpticalSiPM_SD::EndOfEvent(G4HCofThisEvent*)
{
    auto analysis = G4AnalysisManager::Instance();
    auto event = G4RunManager::GetRunManager()->GetCurrentEvent();
    G4int eventID = event->GetEventID();

    analysis->FillNtupleIColumn(3, 0, eventID);     // Column 0: EventID
    analysis->FillNtupleIColumn(3, 1, fPhotonCount); // Column 1: nPhotons
    analysis->AddNtupleRow(3);
}

