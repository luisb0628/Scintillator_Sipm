#include "EventAction.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4ios.hh"

#include <chrono>
#include <iomanip>

EventAction::EventAction()
    : G4UserEventAction(), fTotalEvents(0), fPrintEvery(1),
      fRunStart(std::chrono::steady_clock::now())
{}

void EventAction::BeginOfEventAction(const G4Event* event)
{
    const G4Run* run = G4RunManager::GetRunManager()->GetCurrentRun();

    // Inicializar en el primer evento
    if (event->GetEventID() == 0) {
        fTotalEvents = run->GetNumberOfEventToBeProcessed();
        fPrintEvery  = std::max(1, fTotalEvents / 100); // cada 1%
        fRunStart    = std::chrono::steady_clock::now();
        G4cout << "\n[Progreso] Iniciando " << fTotalEvents << " eventos...\n" << G4endl;
    }

    G4int evID = event->GetEventID();
    if (evID % fPrintEvery != 0) return;

    auto now     = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - fRunStart).count();
    double pct   = (fTotalEvents > 0) ? 100.0 * evID / fTotalEvents : 0.0;

    // Estimar tiempo restante
    std::string eta_str = "--:--";
    if (evID > 0 && fTotalEvents > 0) {
        double rate    = evID / elapsed;           // eventos/s
        double remaining = (fTotalEvents - evID) / rate;
        int h = (int)(remaining / 3600);
        int m = (int)(remaining / 60) % 60;
        int s = (int)(remaining) % 60;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
        eta_str = buf;
    }

    // Barra de progreso (20 chars)
    int filled = (int)(pct / 5);
    std::string bar = "[" + std::string(filled, '=') + std::string(20-filled, ' ') + "]";

    int h = (int)(elapsed / 3600);
    int m = (int)(elapsed / 60) % 60;
    int s = (int)(elapsed) % 60;

    G4cout << std::fixed << std::setprecision(1)
           << bar << " " << std::setw(5) << pct << "%"
           << "  Evento " << std::setw(7) << evID << "/" << fTotalEvents
           << "  Elapsed: " << h << "h" << std::setw(2) << std::setfill('0')
           << m << "m" << std::setw(2) << s << "s"
           << std::setfill(' ')
           << "  ETA: " << eta_str
           << G4endl;
}
