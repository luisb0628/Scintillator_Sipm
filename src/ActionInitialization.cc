#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"

ActionInitialization::ActionInitialization()
: G4VUserActionInitialization()
{
    // Registrar en el hilo master para que el macro pueda usar /gun/ antes de /run/initialize.
    // Geant4 MT repite estos comandos en cada hilo worker al inicializarse.
    fMessenger = new G4GenericMessenger(this, "/gun/",
                                        "Configuracion de la fuente de particulas");

    fMessenger->DeclareProperty("inputFile", fInputFile,
        "Phase space: ruta a output.root de Film_graphene (ntuple ProductosCaptura o ParticulasTransmitidas)");

    fMessenger->DeclareProperty("ntuple", fNtupleName,
        "Nombre del ntuple a leer en inputFile: ParticulasTransmitidas (default) o ProductosCaptura");

    fMessenger->DeclareProperty("source", fSource,
        "Fuente calibracion preset: Co60 (1173+1332 keV), Sr90 (espectro beta)");
}

ActionInitialization::~ActionInitialization()
{
    delete fMessenger;
}

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction());
}

void ActionInitialization::Build() const
{
    auto* gen = new PrimaryGeneratorAction();

    // Si se especificó archivo, activa modo kFile.
    if (!fInputFile.empty()) gen->SetInputFile(fInputFile);

    // Nombre de ntuple (opcional, default "ParticulasTransmitidas" en PrimaryGeneratorAction).
    if (!fNtupleName.empty()) gen->SetNtupleName(fNtupleName);

    // Si se especificó fuente preset (Co60, Sr90), activa el modo correspondiente.
    // Tiene precedencia sobre inputFile si ambos están en el macro.
    if (!fSource.empty())    gen->SetSource(fSource);

    SetUserAction(gen);
    SetUserAction(new RunAction());
    SetUserAction(new EventAction());
}
