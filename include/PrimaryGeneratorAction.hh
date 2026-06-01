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

// Partícula leída de generated_particles.txt (solo productos de captura B-10)
struct InputParticle {
    std::string name;        // "Alpha", "Li7", "Gamma"
    double      energy;      // MeV
    double      dx, dy, dz;
};

// Partícula leída de transmitted_particles.txt (todo lo que llega al detector)
struct TransmittedParticle {
    std::string name;        // nombre Geant4: "gamma", "alpha", "neutron", "e-", "Li7", …
    double      energy;      // MeV
    double      dx, dy, dz;
    std::string vertexVol;   // "graphene", "kapton", "World", … — para filtrar después
    std::string creator;     // proceso creador — para filtrar después
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

    // Comandos de macro
    void SetInputFile(const G4String& path);       // /gun/inputFile       — solo captura B-10
    void SetTransmittedFile(const G4String& path); // /gun/transmittedFile — todo lo transmitido

    G4int GetTotalEvents() const {
        return fTransmittedFile.empty()
            ? (G4int)fEvents.size()
            : (G4int)fTransEvents.size();
    }

private:
    void LoadFile();
    void LoadTransmittedFile();
    G4ParticleDefinition* FindParticleDef(const std::string& name);

    G4ParticleGun*       fParticleGun    = nullptr;
    G4GenericMessenger*  fMessenger      = nullptr;

    // Modo generated_particles.txt (captura B-10)
    G4String             fInputFile;
    G4bool               fLoaded         = false;
    std::vector<std::vector<InputParticle>> fEvents;
    G4int                fCurrentEvent   = 0;

    // Modo transmitted_particles.txt (todo lo que llega al detector)
    G4String             fTransmittedFile;
    G4bool               fTransLoaded    = false;
    std::vector<std::vector<TransmittedParticle>> fTransEvents;
    G4int                fTransCurrentEvent = 0;
};

#endif
