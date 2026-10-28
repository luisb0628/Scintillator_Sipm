#include "PrimaryGeneratorAction.hh"

#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4IonTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4Event.hh"
#include "G4ios.hh"
#include "G4AutoLock.hh"
#include "Randomize.hh"

#include "TFile.h"
#include "TTree.h"

#include <unordered_map>
#include <cctype>
#include <cmath>

// ── Estado compartido entre todos los hilos worker ──────────────────────────
// Ver comentario en el header: el phase space se carga UNA vez y se comparte
// (solo lectura tras la carga) en vez de una copia por hilo.
G4Mutex  PrimaryGeneratorAction::sLoadMutex = G4MUTEX_INITIALIZER;
G4bool   PrimaryGeneratorAction::sLoaded    = false;
G4String PrimaryGeneratorAction::sLoadedFile;
G4String PrimaryGeneratorAction::sLoadedNtuple;
std::vector<std::vector<PhaseSpaceParticle>> PrimaryGeneratorAction::sEvents;

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
    fMode      = GunMode::kFile;
    // La carga real (y el reset de sEvents si el archivo cambio) ocurre en
    // LoadFile(), protegida por sLoadMutex — ver comentario en el header.
}

// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::SetNtupleName(const G4String& name)
{
    fNtupleName = name;
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
// Lee output.root de Film_graphene (ntuple "ParticulasTransmitidas" o
// "ProductosCaptura", ver RunAction.cc de neutron-capture-graphene-boron-geant4):
//   EventID(int)  ParticleType(string)  KineticEnergy_MeV(double)  DirX DirY DirZ ...
//
// Se agrupa por EventID con un unordered_map (no por adyacencia de filas):
// Film_graphene corre en modo MT, así que las filas de un mismo evento pueden
// llegar intercaladas entre hilos al fusionar el ntuple — asumir orden habría
// partido eventos con varias secundarias (alpha + gamma, etc.) en grupos sueltos.
// ---------------------------------------------------------------------------
void PrimaryGeneratorAction::LoadFile()
{
    G4AutoLock lock(&sLoadMutex);

    // Otro hilo ya cargo este mismo archivo/ntuple mientras esperabamos el lock.
    if (sLoaded && sLoadedFile == fInputFile && sLoadedNtuple == fNtupleName) return;

    if (fInputFile.empty()) {
        G4Exception("PrimaryGeneratorAction::LoadFile", "NoFile",
                    FatalException,
                    "No se especifico archivo de phase space.\n"
                    "Usa en el macro:\n"
                    "  /gun/inputFile ../../neutron-capture-graphene-boron-geant4/build/output.root\n"
                    "  /gun/ntuple ParticulasTransmitidas   (o ProductosCaptura)");
    }

    TFile* file = TFile::Open(fInputFile.c_str(), "READ");
    if (!file || file->IsZombie()) {
        G4String msg = "No se puede abrir: " + fInputFile;
        if (file) { file->Close(); delete file; }
        G4Exception("PrimaryGeneratorAction::LoadFile", "FileError",
                    FatalException, msg);
    }

    TTree* tree = dynamic_cast<TTree*>(file->Get(fNtupleName.c_str()));
    if (!tree) {
        G4String msg = "Ntuple '" + fNtupleName + "' no encontrado en " + fInputFile;
        file->Close(); delete file;
        G4Exception("PrimaryGeneratorAction::LoadFile", "NtupleError",
                    FatalException, msg);
    }

    G4cout << "\n[PrimaryGenerator] Cargando " << fInputFile
           << "  (ntuple: " << fNtupleName << ")" << G4endl;

    int    evID;
    char   particleType[64];
    double kinE, dirX, dirY, dirZ, posX, posY;

    tree->SetBranchAddress("EventID",           &evID);
    tree->SetBranchAddress("ParticleType",      particleType);
    tree->SetBranchAddress("KineticEnergy_MeV", &kinE);
    tree->SetBranchAddress("DirX",              &dirX);
    tree->SetBranchAddress("DirY",              &dirY);
    tree->SetBranchAddress("DirZ",              &dirZ);
    tree->SetBranchAddress("PosX_cm",           &posX);
    tree->SetBranchAddress("PosY_cm",           &posY);

    std::unordered_map<int, std::vector<PhaseSpaceParticle>> grouped;
    Long64_t nEntries = tree->GetEntries();

    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        if (kinE <= 0.) continue;

        PhaseSpaceParticle p;
        p.name   = particleType;
        p.energy = kinE;
        p.dx = dirX; p.dy = dirY; p.dz = dirZ;
        p.x  = posX; p.y  = posY;
        grouped[evID].push_back(p);
    }

    file->Close();
    delete file;

    sEvents.clear();
    sEvents.reserve(grouped.size());
    for (auto& kv : grouped) sEvents.push_back(std::move(kv.second));

    if (sEvents.empty()) {
        G4Exception("PrimaryGeneratorAction::LoadFile", "EmptyFile",
                    FatalException,
                    "El ntuple no contiene eventos validos (E>0).");
    }

    G4cout << "[PrimaryGenerator] Grupos cargados: " << sEvents.size() << G4endl;
    sLoadedFile   = fInputFile;
    sLoadedNtuple = fNtupleName;
    sLoaded       = true;
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
    // Archivo (captura B-10): film pegado al centellador → 10 µm de aire; la posicion (x, y) de cada
    // particula es la del plano del detector de la etapa 2, con el eje del film en el eje del cristal.
    // Calibración: fuente externa → 1 mm de aire (irrelevante para gammas/e-).
    const G4double      gunZFile = -0.01*mm;            // ~10 µm
    const G4ThreeVector gunPosCalib(0., 0., -1.*mm);
    const G4ThreeVector gunDir(0., 0., 1.);

    // ── Modo archivo: output.root de Film_graphene (ProductosCaptura/ParticulasTransmitidas) ──
    if (fMode == GunMode::kFile) {
        if (!sLoaded || sLoadedFile != fInputFile || sLoadedNtuple != fNtupleName) LoadFile();

        const auto& grp = sEvents[event->GetEventID() % (G4int)sEvents.size()];
        for (const auto& p : grp) {
            G4ParticleDefinition* def = FindParticleDef(p.name);
            if (!def) {
                G4cout << "[PrimaryGenerator] Particula ignorada: " << p.name << G4endl;
                continue;
            }
            fParticleGun->SetParticleDefinition(def);
            fParticleGun->SetParticleEnergy(p.energy * MeV);
            fParticleGun->SetParticleMomentumDirection(G4ThreeVector(p.dx, p.dy, p.dz));
            fParticleGun->SetParticlePosition(G4ThreeVector(p.x*cm, p.y*cm, gunZFile));
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
