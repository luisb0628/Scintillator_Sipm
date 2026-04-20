#ifndef ScintSD_h
#define ScintSD_h 1

#include "G4VSensitiveDetector.hh"
#include "globals.hh"
#include <map>

class G4Step;
class G4TouchableHistory;

class ScintSD : public G4VSensitiveDetector
{
public:
    ScintSD(const G4String& name);
    ~ScintSD() override;

    void Initialize(G4HCofThisEvent*) override;
    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;
    void EndOfEvent(G4HCofThisEvent*) override;

private:
    // Edep acumulado por tipo de partícula (por evento)
    G4double fEdepTotal   = 0.;
    G4double fEdepAlpha   = 0.;
    G4double fEdepLi7     = 0.;
    G4double fEdepGamma   = 0.;   // gammas + electrones secundarios

    // Conteo de fotones ópticos generados en este evento
    G4int fNOptPhotons = 0;

    // Edep por TrackID (para ntuple 0 detallado)
    std::map<G4int, G4double> fEdepPerTrack;
};

#endif
