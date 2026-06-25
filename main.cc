#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4Threading.hh"
#include "Randomize.hh"

#include "FTFP_BERT.hh"
#include "G4OpticalPhysics.hh"

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"

int main(int argc, char** argv)
{
    G4Random::setTheSeed(123456789);

    G4UIExecutive* ui = nullptr;
    if (argc == 1)
        ui = new G4UIExecutive(argc, argv);

    auto* runManager = new G4MTRunManager();
    runManager->SetNumberOfThreads(G4Threading::G4GetNumberOfCores());

    // Detector — tipo configurable desde macro: /scint/type BGO
    runManager->SetUserInitialization(new DetectorConstruction());

    // Física EM + hadrónica + óptica.
    // FTFP_BERT es mucho más rápido de inicializar que QGSP_BERT_HP porque
    // no carga las tablas de sección eficaz de neutrones de alta precisión.
    // Los neutrones ya fueron transportados en Film_graphene; aquí solo
    // necesitamos EM (gammas/e-), iones (α, Li7) y fotones ópticos.
    auto* physicsList = new FTFP_BERT();
    physicsList->SetVerboseLevel(0);
    physicsList->RegisterPhysics(new G4OpticalPhysics());
    runManager->SetUserInitialization(physicsList);

    runManager->SetUserInitialization(new ActionInitialization());

    auto* visManager = new G4VisExecutive();
    visManager->Initialize();

    auto* UI = G4UImanager::GetUIpointer();

    if (!ui) {
        UI->ApplyCommand("/control/execute " + G4String(argv[1]));
    } else {
        UI->ApplyCommand("/control/execute macros/vis1.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;
    return 0;
}
