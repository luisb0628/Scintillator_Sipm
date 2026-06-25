#pragma once

#include <G4VSensitiveDetector.hh>
#include "globals.hh"

class OpticalSiPM_SD : public G4VSensitiveDetector
{
public:
    OpticalSiPM_SD(const G4String& name);
    virtual ~OpticalSiPM_SD() = default;

    virtual G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;
    virtual void Initialize(G4HCofThisEvent*) override;
    virtual void EndOfEvent(G4HCofThisEvent*) override;

private:
    G4int    fPhotonCount  = 0;
    G4double fPDE          = 0.30;

    // Fotoelectrones por categoría de partícula que generó el fotón óptico
    G4int fPE_alpha    = 0;
    G4int fPE_Li7      = 0;
    G4int fPE_gamma    = 0;
    G4int fPE_electron = 0;
    G4int fPE_neutron  = 0;
    G4int fPE_proton   = 0;
    G4int fPE_other    = 0;
};
