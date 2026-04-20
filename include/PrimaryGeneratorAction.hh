#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4GenericMessenger.hh"
#include "globals.hh"

#include <vector>
#include <string>

class G4ParticleTable;
class G4Event;

// Una partícula leída del archivo ASCII de Film_graphene
struct InputParticle {
    std::string name;   // "Alpha", "Li7", "Gamma"
    double      energy; // MeV
    double      dx, dy, dz; // dirección
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

    // Comando de macro: /gun/inputFile <path>
    void SetInputFile(const G4String& path);

    G4int GetTotalEvents() const { return (G4int)fEvents.size(); }

private:
    void LoadFile();

    G4ParticleGun*       fParticleGun    = nullptr;
    G4GenericMessenger*  fMessenger      = nullptr;
    G4String             fInputFile;
    G4bool               fLoaded         = false;

    // Todos los eventos leídos del archivo
    std::vector<std::vector<InputParticle>> fEvents;
    G4int fCurrentEvent = 0;
};

#endif
