#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4GenericMessenger.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4Material;
class G4LogicalVolume;

enum class ScintType { PLASTIC, BGO, CSI, LYSO };

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // Configurable desde macro: /scint/type BGO|PLASTIC|CSI|LYSO
    void SetScintTypeByName(const G4String& name);

private:
    G4Material* BuildScintMaterial();
    void        AddOpticalProperties(G4Material* mat);
    void        DefineCommands();

    ScintType  fScintType  = ScintType::BGO;

    G4LogicalVolume* fLogicScint = nullptr;
    G4LogicalVolume* fLogicSiPM  = nullptr;

    G4GenericMessenger* fMessenger = nullptr;
};

#endif
