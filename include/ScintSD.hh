#ifndef ScintSD_h
#define ScintSD_h 1

#include "G4VSensitiveDetector.hh"
#include "G4Threading.hh"
#include "globals.hh"
#include <unordered_map>

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

    // trackID → categoría canónica ("alpha","Li7","gamma","electron","neutron","proton","other")
    static G4ThreadLocal std::unordered_map<G4int, G4String>* sTrackParticle;

    // trackID → parentTrackID — permite subir el árbol hasta la primaria
    static G4ThreadLocal std::unordered_map<G4int, G4int>*   sTrackParent;

    // Devuelve la categoría de la partícula PRIMARIA que originó trackID
    // (sube el árbol de parentesco hasta parentID == 0)
    static G4String GetPrimaryForTrack(G4int trackID);

private:
    G4double fEdepTotal   = 0.;
    G4int    fNOptPhotons = 0;
};

#endif
