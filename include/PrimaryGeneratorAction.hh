#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

#include <vector>
#include <string>

class G4Event;

// Partícula de phase space — formato común a generated_particles.txt y transmitted_particles.txt:
//   EventID Name Ekin_MeV DirX DirY DirZ PosX_cm PosY_cm PosZ_cm Creator TargetZ TargetA
struct PhaseSpaceParticle {
    std::string name;
    double      energy;    // MeV
    double      dx, dy, dz;
};

enum class GunMode {
    kStandard,  // /gun/particle + /gun/energy (Cs137 u otro)
    kFile,      // phase space desde archivo (generated o transmitted)
    kCo60,      // dos gammas en cascada: 1173.2 + 1332.5 keV
    kSr90       // espectro beta Sr-90/Y-90 en equilibrio secular
};

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

    // Llamados desde ActionInitialization::Build() según los comandos del macro
    void SetInputFile(const G4String& path);          // /gun/inputFile
    void SetSource(const G4String& source);           // /gun/source Co60|Sr90

    G4int GetTotalEvents() const { return (G4int)fEvents.size(); }

private:
    void LoadFile();
    G4double SampleBetaEnergy(G4double Q, G4double fmax);
    G4ParticleDefinition* FindParticleDef(const std::string& name);

    G4ParticleGun*  fParticleGun  = nullptr;
    GunMode         fMode         = GunMode::kStandard;

    G4String        fInputFile;
    G4bool          fLoaded       = false;
    std::vector<std::vector<PhaseSpaceParticle>> fEvents;

    // Máximos del espectro beta pre-calculados en SetSource("Sr90")
    G4double        fBetaMaxSr90  = 0.;
    G4double        fBetaMaxY90   = 0.;
};

#endif
