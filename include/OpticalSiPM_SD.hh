#pragma once

#include <G4VSensitiveDetector.hh>

class OpticalSiPM_SD : public G4VSensitiveDetector
{
public:
    OpticalSiPM_SD(const G4String& name);
    virtual ~OpticalSiPM_SD() = default;

    virtual G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;
    virtual void Initialize(G4HCofThisEvent*) override;
    virtual void EndOfEvent(G4HCofThisEvent*) override;

private:
    G4int fPhotonCount;        // número total de fotones detectados en este evento
    G4double fPDE = 0.30;      // Photo Detection Efficiency (30%)
};
