#ifndef ActionInitialization_h
#define ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"
#include "G4GenericMessenger.hh"
#include "globals.hh"

class ActionInitialization : public G4VUserActionInitialization {
  public:
    ActionInitialization();
    ~ActionInitialization() override;

    void BuildForMaster() const override;
    void Build() const override;

  private:
    G4GenericMessenger* fMessenger = nullptr;
    mutable G4String    fInputFile;   // /gun/inputFile  — ruta a output.root de Film_graphene
    mutable G4String    fNtupleName;  // /gun/ntuple     — ParticulasTransmitidas (default) o ProductosCaptura
    mutable G4String    fSource;      // /gun/source     — Co60, Sr90
};

#endif
