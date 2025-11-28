// DetectorConstruction.hh
#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4Material;

enum class ScintType {
    PLASTIC,
    BGO,
    CSI,
    LYSO
};

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // Setter (sin macro, lo usas desde main.cc)
    void SetScintType(ScintType t) { fScintType = t; }

private:
    G4Material* CreateScintillatorMaterial();
    void DefineOpticalProperties(G4Material* scintMat);

    ScintType fScintType;

    // Para guardar punteros a volúmenes físicos si quieres más adelante
    G4VPhysicalVolume* fPhysWorld = nullptr;
};

#endif
