#include "ScintSD.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpticalPhoton.hh"
#include "G4VProcess.hh"

G4ThreadLocal std::unordered_map<G4int, G4String>* ScintSD::sTrackParticle = nullptr;
G4ThreadLocal std::unordered_map<G4int, G4int>*    ScintSD::sTrackParent   = nullptr;

ScintSD::ScintSD(const G4String& name)
    : G4VSensitiveDetector(name)
{}

ScintSD::~ScintSD() {}

// Categoría canónica de partícula para los mapas de tracking
static G4String Categorize(const G4String& pname)
{
    if (pname == "alpha")                               return "alpha";
    if (pname.size() >= 3 && pname.substr(0,3)=="Li7") return "Li7";
    if (pname == "gamma")                               return "gamma";
    if (pname == "e-" || pname == "e+")                return "electron";
    if (pname == "neutron")                             return "neutron";
    if (pname == "proton")                              return "proton";
    return "other";
}

// ---------------------------------------------------------------------------
// Sube el árbol de parentesco hasta encontrar la partícula con parentID == 0
// (la primaria original del evento) y devuelve su categoría.
// Con esto, un gamma de 478 keV → Compton e- → fotón óptico queda como "gamma",
// no como "electron".
// ---------------------------------------------------------------------------
G4String ScintSD::GetPrimaryForTrack(G4int trackID)
{
    if (!sTrackParticle || !sTrackParent) return "other";

    G4int id = trackID;
    for (int depth = 0; depth < 200; ++depth) {
        auto pit = sTrackParent->find(id);
        if (pit == sTrackParent->end() || pit->second == 0) {
            // id tiene parentID=0 (es primaria) o no está registrado
            auto cit = sTrackParticle->find(id);
            return (cit != sTrackParticle->end()) ? cit->second : "other";
        }
        id = pit->second;
    }
    return "other";
}

// ─── Initialize — inicio de cada evento ──────────────────────────────────────
void ScintSD::Initialize(G4HCofThisEvent*)
{
    fEdepTotal   = 0.;
    fNOptPhotons = 0;

    if (!sTrackParticle)
        sTrackParticle = new std::unordered_map<G4int, G4String>();
    sTrackParticle->clear();

    if (!sTrackParent)
        sTrackParent = new std::unordered_map<G4int, G4int>();
    sTrackParent->clear();
}

// ─── ProcessHits ─────────────────────────────────────────────────────────────
G4bool ScintSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    auto* track = step->GetTrack();

    // Contar fotones ópticos nacidos en este step
    for (auto* sec : *step->GetSecondaryInCurrentStep()) {
        if (sec->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
            ++fNOptPhotons;
    }

    // Los fotones ópticos no depositan energía ni se registran en los mapas
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
        return false;

    // Registrar trackID → (categoría, parentID).
    // Se llama en cada step pero emplace no sobreescribe si la clave ya existe,
    // así que solo registra la primera vez que el track pasa por el SD.
    // NO se usa GetCurrentStepNumber()==1 porque ese contador es global para
    // el track: un gamma disparado desde fuera del centellador tiene paso 1 en
    // aire y llega al SD en paso 2 o mayor → con la condición nunca se registra.
    sTrackParticle->emplace(track->GetTrackID(),
                            Categorize(track->GetDefinition()->GetParticleName()));
    sTrackParent->emplace(track->GetTrackID(), track->GetParentID());

    G4double edep = step->GetTotalEnergyDeposit();
    if (edep <= 0.) return false;

    fEdepTotal += edep;
    return true;
}

// ─── EndOfEvent ───────────────────────────────────────────────────────────────
void ScintSD::EndOfEvent(G4HCofThisEvent*)
{
    auto* am  = G4AnalysisManager::Instance();
    G4int evID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // Ntuple 0 — ScintEvent: TotalEdep + nOptPhotons por evento
    am->FillNtupleIColumn(0, 0, evID);
    am->FillNtupleDColumn(0, 1, fEdepTotal / keV);
    am->FillNtupleIColumn(0, 2, fNOptPhotons);
    am->AddNtupleRow(0);
}
