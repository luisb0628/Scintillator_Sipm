#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include <chrono>

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    virtual ~EventAction() = default;

    virtual void BeginOfEventAction(const G4Event*);

private:
    G4int fTotalEvents;
    G4int fPrintEvery;
    std::chrono::steady_clock::time_point fRunStart;
};

#endif
