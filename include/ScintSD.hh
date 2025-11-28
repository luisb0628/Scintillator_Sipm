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
    virtual ~ScintSD();

    virtual void Initialize(G4HCofThisEvent*);
    virtual G4bool ProcessHits(G4Step*, G4TouchableHistory*);
    virtual void EndOfEvent(G4HCofThisEvent*);

private:
    // Acumulador de energía por TrackID en este evento
    std::map<G4int, G4double> fEdepPerTrack;
};

#endif
