#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4ScoringManager.hh"
#include "Randomize.hh"

// Física
#include "QGSP_BIC_HP.hh"
#include "G4OpticalPhysics.hh"

// Usuario
#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"

int main(int argc, char** argv) 
{
    G4Random::setTheSeed(123456789);

    try {
        // Modo UI
        G4UIExecutive* ui = nullptr;
        if (argc == 1) {
            ui = new G4UIExecutive(argc, argv);
        }

        // Run Manager
        auto* runManager = new G4RunManager();

        // Scoring
        G4ScoringManager::GetScoringManager();

        // Detector
        auto* detector = new DetectorConstruction();

        // Elegir centellador (sin macros)
        detector->SetScintType(ScintType::PLASTIC);

        runManager->SetUserInitialization(detector);

        // Física Hadron + Física Óptica
        auto* physicsList = new QGSP_BIC_HP();
        physicsList->SetVerboseLevel(0);
        // ----------------------------------
        // DESACTIVAR RADIOACTIVE DECAY
        // ----------------------------------
        physicsList->RemovePhysics("G4DecayPhysics");
        physicsList->RemovePhysics("G4RadioactiveDecay");

        // Física óptica (NADA MÁS)
        auto* opticalPhysics = new G4OpticalPhysics();
        physicsList->RegisterPhysics(opticalPhysics);

        runManager->SetUserInitialization(physicsList);

        // Actions
        runManager->SetUserInitialization(new ActionInitialization());

        // Inicializar G4
        runManager->Initialize();

        // Visualización
        auto* visManager = new G4VisExecutive();
        visManager->Initialize();

        // UI manager
        auto* UImanager = G4UImanager::GetUIpointer();

        UImanager->ApplyCommand("/control/verbose 1");
        UImanager->ApplyCommand("/run/verbose 0");
        UImanager->ApplyCommand("/event/verbose 0");
        UImanager->ApplyCommand("/tracking/verbose 0");

        // Batch o interactivo
        if (!ui) {
            G4String command = "/control/execute ";
            G4String macro = argv[1];
            UImanager->ApplyCommand(command + macro);
        }
        else {
            UImanager->ApplyCommand("/control/execute ../macros/vis1.mac");
            ui->SessionStart();
            delete ui;
        }

        // Limpieza
        delete visManager;
        delete runManager;
    }
    catch (...) {
        G4cerr << "ERROR: Excepción en ejecución.\n";
        return 1;
    }

    return 0;
}
