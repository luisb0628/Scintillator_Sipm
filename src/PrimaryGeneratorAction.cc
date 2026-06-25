#include "PrimaryGeneratorAction.hh"

#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ios.hh"
#include "Randomize.hh"

#include <fstream>
#include <sstream>
#include <cctype>
#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction()
    : G4VUserPrimaryGeneratorAction()
{
    fParticleGun = new G4ParticleGun(1);

    // Default modo estándar: gamma 662 keV (Cs-137), apuntando al centellador.
    // Puede cambiarse con /gun/particle y /gun/energy en el macro.
    auto* table = G4ParticleTable::GetParticleTable();
    fParticleGun->SetParticleDefinition(table->FindParticle("gamma"));
    fParticleGun->SetParticleEnergy(662. * keV);
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
    fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., -1.*mm));
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::SetInputFile(const G4String& path)
{
    fInputFile = path;
    fLoaded    = false;
    fEvents.clear();
    fMode      = GunMode::kFile;
}

// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::SetSource(const G4String& src)
{
    // Calcula el máximo del espectro beta N(T) ∝ p·E·(Q-T)²
    // escaneando 1000 puntos — se llama una vez en Build(), no por evento.
    auto computeMax = [](G4double Q) -> G4double {
        const G4double me = 0.511;
        G4double fmax = 0.;
        for (int i = 1; i < 1000; ++i) {
            G4double T  = Q * i / 1000.;
            G4double E  = T + me;
            G4double p2 = E*E - me*me;
            G4double f  = std::sqrt(p2) * E * (Q-T)*(Q-T);
            if (f > fmax) fmax = f;
        }
        return fmax * 1.02; // 2% margen para garantizar que el max real quede cubierto
    };

    if (src == "Cs137") {
        // El constructor ya configura gamma 662 keV apuntando al centellador.
        // kStandard simplemente usa ese gun sin modificarlo.
        fMode = GunMode::kStandard;

    } else if (src == "Co60") {
        fMode = GunMode::kCo60;

    } else if (src == "Sr90") {
        fMode        = GunMode::kSr90;
        fBetaMaxSr90 = computeMax(0.546);   // Sr-90: Q_β = 0.546 MeV
        fBetaMaxY90  = computeMax(2.280);   // Y-90:  Q_β = 2.280 MeV

    } else {
        G4cerr << "\n[PrimaryGenerator] Fuente desconocida: \"" << src << "\"\n"
               << "   Opciones validas: Cs137, Co60, Sr90\n"
               << "   Para otras fuentes puntuales usa /gun/particle y /gun/energy.\n";
    }
}

// ---------------------------------------------------------------------------
// Lee generated_particles.txt o transmitted_particles.txt.
// Formato común (ambos archivos desde la Etapa 2):
//   EventID Name Ekin_MeV DirX DirY DirZ PosX_cm PosY_cm PosZ_cm Creator TargetZ TargetA
// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::LoadFile()
{
    if (fInputFile.empty()) {
        G4Exception("PrimaryGeneratorAction::LoadFile", "NoFile",
                    FatalException,
                    "No se especifico archivo de phase space.\n"
                    "Usa en el macro:\n"
                    "  /gun/inputFile ../build/macros/generated_particles.txt\n"
                    "  /gun/inputFile ../build/macros/transmitted_particles.txt");
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
    std::vector<PhaseSpaceParticle> group;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        int    evID;
        std::string pname;
        double ekin, dx, dy, dz, px, py, pz;
        std::string creator;
        int tz, ta;

        if (!(ss >> evID >> pname >> ekin >> dx >> dy >> dz
                 >> px >> py >> pz >> creator >> tz >> ta))
            continue;

        if (ekin <= 0.) continue;

        if (evID != prevID) {
            if (!group.empty()) fEvents.push_back(group);
            group.clear();
            prevID = evID;
        }

        PhaseSpaceParticle p;
        p.name   = pname;
        p.energy = ekin;
        p.dx = dx; p.dy = dy; p.dz = dz;
        group.push_back(p);
    }
    if (!group.empty()) fEvents.push_back(group);

    if (fEvents.empty()) {
        G4Exception("PrimaryGeneratorAction::LoadFile", "EmptyFile",
                    FatalException,
                    "El archivo no contiene eventos validos (E>0).");
    }

    G4cout << "[PrimaryGenerator] Grupos cargados: " << fEvents.size() << G4endl;
    fLoaded = true;
}

// ---------------------------------------------------------------------------
G4ParticleDefinition* PrimaryGeneratorAction::FindParticleDef(const std::string& name)
{
    auto* table = G4ParticleTable::GetParticleTable();

    std::string lname = name;
    for (auto& c : lname) c = (char)std::tolower((unsigned char)c);

    if (lname == "alpha")    return G4IonTable::GetIonTable()->GetIon(2, 4, 0.);
    if (lname == "li7")      return G4IonTable::GetIonTable()->GetIon(3, 7, 0.);
    if (lname == "li6")      return G4IonTable::GetIonTable()->GetIon(3, 6, 0.);
    if (lname == "he3")      return G4IonTable::GetIonTable()->GetIon(2, 3, 0.);
    if (lname == "triton")   return G4IonTable::GetIonTable()->GetIon(1, 3, 0.);
    if (lname == "deuteron") return G4IonTable::GetIonTable()->GetIon(1, 2, 0.);

    G4ParticleDefinition* def = table->FindParticle(name);
    if (!def) def = table->FindParticle(lname);
    return def;
}

// ---------------------------------------------------------------------------
// Muestreo por rechazo del espectro beta permitido (sin factor de Fermi):
//   N(T) ∝ sqrt(T² + 2·T·mₑ) · (T + mₑ) · (Q - T)²
// fmax debe ser el máximo real de la función (pre-calculado en SetSource).
// ---------------------------------------------------------------------------
G4double PrimaryGeneratorAction::SampleBetaEnergy(G4double Q, G4double fmax)
{
    const G4double me = 0.511;
    G4double T, f;
    do {
        T = G4UniformRand() * Q;
        G4double E  = T + me;
        G4double p2 = E*E - me*me;
        f = (p2 > 0.) ? std::sqrt(p2) * E * (Q-T)*(Q-T) : 0.;
    } while (G4UniformRand() * fmax > f);
    return T;
}

// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    // Archivo (captura B-10): film pegado al centellador → 10 µm de aire.
    // Calibración: fuente externa → 1 mm de aire (irrelevante para gammas/e-).
    const G4ThreeVector gunPosFile(0., 0., -0.01*mm);   // ~10 µm
    const G4ThreeVector gunPosCalib(0., 0., -1.*mm);
    const G4ThreeVector gunDir(0., 0., 1.);

    // ── Modo archivo: generated_particles.txt o transmitted_particles.txt ────
    if (fMode == GunMode::kFile) {
        if (!fLoaded) LoadFile();

        const auto& grp = fEvents[event->GetEventID() % (G4int)fEvents.size()];
        for (const auto& p : grp) {
            G4ParticleDefinition* def = FindParticleDef(p.name);
            if (!def) {
                G4cout << "[PrimaryGenerator] Particula ignorada: " << p.name << G4endl;
                continue;
            }
            fParticleGun->SetParticleDefinition(def);
            fParticleGun->SetParticleEnergy(p.energy * MeV);
            fParticleGun->SetParticleMomentumDirection(G4ThreeVector(p.dx, p.dy, p.dz));
            fParticleGun->SetParticlePosition(gunPosFile);
            fParticleGun->GeneratePrimaryVertex(event);
        }
        return;
    }

    // ── Co-60: dos gammas en cascada 1173.2 + 1332.5 keV ────────────────────
    if (fMode == GunMode::kCo60) {
        auto* gamma = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
        fParticleGun->SetParticleDefinition(gamma);
        fParticleGun->SetParticlePosition(gunPosCalib);
        fParticleGun->SetParticleMomentumDirection(gunDir);

        fParticleGun->SetParticleEnergy(1173.2 * keV);
        fParticleGun->GeneratePrimaryVertex(event);

        fParticleGun->SetParticleEnergy(1332.5 * keV);
        fParticleGun->GeneratePrimaryVertex(event);
        return;
    }

    // ── Sr-90/Y-90: espectro beta en equilibrio secular ───────────────────────
    // 50% Sr-90 (Q = 0.546 MeV) + 50% Y-90 (Q = 2.280 MeV), un beta por evento
    if (fMode == GunMode::kSr90) {
        auto* electron = G4ParticleTable::GetParticleTable()->FindParticle("e-");
        fParticleGun->SetParticleDefinition(electron);
        fParticleGun->SetParticlePosition(gunPosCalib);
        fParticleGun->SetParticleMomentumDirection(gunDir);

        G4double T = (G4UniformRand() < 0.5)
            ? SampleBetaEnergy(0.546, fBetaMaxSr90)
            : SampleBetaEnergy(2.280, fBetaMaxY90);

        fParticleGun->SetParticleEnergy(T * MeV);
        fParticleGun->GeneratePrimaryVertex(event);
        return;
    }

    // ── Modo estándar: /gun/particle, /gun/energy, etc. ──────────────────────
    fParticleGun->GeneratePrimaryVertex(event);
}
