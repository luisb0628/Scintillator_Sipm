#include "PrimaryGeneratorAction.hh"

#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ios.hh"

#include <fstream>
#include <sstream>

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction()
{
    fParticleGun = new G4ParticleGun(1);

    // Configuración por defecto para modo sin archivo (calibración):
    // gamma de 662 keV (equivalente a Cs-137) disparado hacia el centellador.
    // Se puede cambiar desde el macro con los comandos estándar /gun/*.
    auto* table = G4ParticleTable::GetParticleTable();
    fParticleGun->SetParticleDefinition(table->FindParticle("gamma"));
    fParticleGun->SetParticleEnergy(662. * keV);
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
    fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., -1.*cm));

    fMessenger = new G4GenericMessenger(this, "/gun/",
                                        "Configuracion de la fuente de particulas");
    fMessenger->DeclareMethod("inputFile",
        &PrimaryGeneratorAction::SetInputFile,
        "Ruta al archivo generated_particles.txt de Film_graphene")
        .SetParameterName("path", false);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
    delete fMessenger;
}

void PrimaryGeneratorAction::SetInputFile(const G4String& path)
{
    fInputFile    = path;
    fLoaded       = false;
    fCurrentEvent = 0;
    fEvents.clear();
}

// ---------------------------------------------------------------------------
// Lee generated_particles.txt y agrupa las partículas por EventID.
// Formato de cada línea (Film_graphene CaptureSD):
//   EventID Particle KinEnergy_MeV Edep_MeV DirX DirY DirZ StepLen_um Creator TargetZ TargetA
// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::LoadFile()
{
    if (fInputFile.empty()) {
        G4Exception("PrimaryGeneratorAction::LoadFile", "NoFile",
                    FatalException,
                    "No se especifico archivo de entrada.\n"
                    "Usa en el macro: /gun/inputFile <ruta>/generated_particles.txt");
    }

    std::ifstream in(fInputFile);
    if (!in.is_open()) {
        G4String msg = "No se puede abrir: " + fInputFile;
        G4Exception("PrimaryGeneratorAction::LoadFile", "FileError",
                    FatalException, msg);
    }

    G4cout << "\n[PrimaryGenerator] Cargando " << fInputFile << G4endl;

    std::string line;
    int prevID = -1;
    std::vector<InputParticle> group;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        int    evID;
        std::string pname;
        double ekin, edep, dx, dy, dz, stepLen;
        std::string creator;
        int tz, ta;

        if (!(ss >> evID >> pname >> ekin >> edep >> dx >> dy >> dz
                 >> stepLen >> creator >> tz >> ta))
            continue;

        if (ekin <= 0.) continue;

        // Solo las partículas relevantes para el centellador
        if (pname != "Alpha" && pname != "Li7" && pname != "Gamma") continue;

        if (evID != prevID) {
            if (!group.empty()) fEvents.push_back(group);
            group.clear();
            prevID = evID;
        }

        InputParticle p;
        p.name   = pname;
        p.energy = ekin;
        p.dx = dx; p.dy = dy; p.dz = dz;
        group.push_back(p);
    }
    if (!group.empty()) fEvents.push_back(group);

    if (fEvents.empty()) {
        G4Exception("PrimaryGeneratorAction::LoadFile", "EmptyFile",
                    FatalException,
                    "El archivo no contiene eventos validos (Alpha/Li7/Gamma con E>0).");
    }

    G4cout << "[PrimaryGenerator] Eventos con captura cargados: "
           << fEvents.size() << G4endl;
    fLoaded = true;
}

// ---------------------------------------------------------------------------
// Por cada evento Geant4 dispara todas las partículas del grupo correspondiente
// (un grupo = un neutrón capturado original en Film_graphene)
// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    // ── Modo fallback: sin archivo → gun estándar (configurable desde macro) ──
    // Útil para calibración: /gun/particle gamma, /gun/energy 662 keV, etc.
    if (fInputFile.empty()) {
        fParticleGun->GeneratePrimaryVertex(event);
        return;
    }

    // ── Modo archivo: usa partículas de Film_graphene ─────────────────────────
    if (!fLoaded) LoadFile();

    const auto& grp = fEvents[fCurrentEvent % (G4int)fEvents.size()];
    ++fCurrentEvent;

    auto* table = G4ParticleTable::GetParticleTable();
    const G4ThreeVector gunPos(0., 0., -0.1*mm);

    for (const auto& p : grp) {
        G4ParticleDefinition* def = nullptr;

        if (p.name == "Alpha") {
            def = G4IonTable::GetIonTable()->GetIon(2, 4, 0.);
        } else if (p.name == "Li7") {
            def = G4IonTable::GetIonTable()->GetIon(3, 7, 0.);
        } else if (p.name == "Gamma") {
            def = table->FindParticle("gamma");
        }

        if (!def) continue;

        fParticleGun->SetParticleDefinition(def);
        fParticleGun->SetParticleEnergy(p.energy * MeV);
        fParticleGun->SetParticleMomentumDirection(G4ThreeVector(p.dx, p.dy, p.dz));
        fParticleGun->SetParticlePosition(gunPos);
        fParticleGun->GeneratePrimaryVertex(event);
    }
}
