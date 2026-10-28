#include "G4MTRunManager.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4Threading.hh"
#include "Randomize.hh"
#include "TROOT.h"

#include "QGSP_BERT_HP.hh"
#include "G4OpticalPhysics.hh"

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"

int main(int argc, char** argv)
{
    G4Random::setTheSeed(123456789);

    // PrimaryGeneratorAction lee output.root (TFile/TTree) de Film_graphene desde
    // el constructor de cada hilo worker; sin esto, aperturas de ROOT concurrentes
    // entre hilos no son seguras.
    ROOT::EnableThreadSafety();

    G4UIExecutive* ui = nullptr;
    if (argc == 1)
        ui = new G4UIExecutive(argc, argv);

    auto* runManager = new G4MTRunManager();
    runManager->SetNumberOfThreads(G4Threading::G4GetNumberOfCores());

    // Detector — tipo configurable desde macro: /scint/type BGO
    runManager->SetUserInitialization(new DetectorConstruction());

    // Física EM + hadrónica + óptica: siempre QGSP_BERT_HP (la misma lista de las
    // tres etapas) más G4OpticalPhysics para el centelleo y los fotones ópticos.
    auto* physicsList = new QGSP_BERT_HP();
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
