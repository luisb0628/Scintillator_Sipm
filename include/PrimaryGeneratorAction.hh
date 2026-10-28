#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4Threading.hh"
#include "globals.hh"

#include <vector>
#include <string>

class G4Event;

// Partícula de phase space — leída del ntuple ROOT generado por Film_graphene
// (output.root, ntuples "ParticulasTransmitidas" o "ProductosCaptura"):
//   EventID  ParticleType  KineticEnergy_MeV  DirX DirY DirZ  PosX_cm PosY_cm ...
struct PhaseSpaceParticle {
    std::string name;
    double      energy;    // MeV
    double      dx, dy, dz;
    double      x, y;      // cm, posicion transversal en el plano del detector de la etapa 2
};

enum class GunMode {
    kStandard,  // /gun/particle + /gun/energy (Cs137 u otro)
    kFile,      // phase space desde output.root (ProductosCaptura o ParticulasTransmitidas)
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
    void SetInputFile(const G4String& path);          // /gun/inputFile   — ruta a output_<config>.root
    void SetNtupleName(const G4String& name);         // /gun/ntuple      — "ParticulasTransmitidas" (default) o "ProductosCaptura"
    void SetSource(const G4String& source);           // /gun/source Co60|Sr90

    G4int GetTotalEvents() const { return (G4int)sEvents.size(); }

private:
    void LoadFile();
    G4double SampleBetaEnergy(G4double Q, G4double fmax);
    G4ParticleDefinition* FindParticleDef(const std::string& name);

    G4ParticleGun*  fParticleGun  = nullptr;
    GunMode         fMode         = GunMode::kStandard;

    G4String        fInputFile;
    G4String        fNtupleName = "ParticulasTransmitidas";

    // fEvents se carga UNA sola vez y se comparte entre todos los hilos worker
    // (cada uno crea su propia instancia de PrimaryGeneratorAction, pero todos
    // apuntan al mismo archivo de phase space). Cargarlo por instancia
    // multiplicaba el uso de memoria por el numero de threads -> OOM ("Killed").
    // Es solo-lectura despues de cargado, asi que el mutex solo protege la carga.
    static G4Mutex  sLoadMutex;
    static G4bool   sLoaded;
    static G4String sLoadedFile;
    static G4String sLoadedNtuple;
    static std::vector<std::vector<PhaseSpaceParticle>> sEvents;

    // Máximos del espectro beta pre-calculados en SetSource("Sr90")
    G4double        fBetaMaxSr90  = 0.;
    G4double        fBetaMaxY90   = 0.;
};

#endif
