#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "G4Threading.hh"
#include "globals.hh"
#include <chrono>

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    virtual ~EventAction() = default;

    virtual void BeginOfEventAction(const G4Event*);

    // Llamado desde RunAction::BeginOfRunAction para resetear el estado
    // entre corridas (sesión interactiva o múltiples /run/beamOn)
    static void ResetProgress();

private:
    // Estado por hilo: se inicializa en el primer evento del hilo
    G4int fTotalEvents = 0;
    G4int fPrintEvery  = 1;

    // Estado compartido entre todos los hilos
    static G4Mutex                               sMutex;
    static G4int                                 sLastMilestone;
    static G4bool                                sStarted;
    static std::chrono::steady_clock::time_point sGlobalStart;
};

#endif
