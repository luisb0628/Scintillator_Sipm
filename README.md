# Simulación de Scintilador + SiPM y Captura de Neutrones en Grafeno Dopado con Boro (Geant4)

Este repositorio contiene el modelo Monte Carlo en **Geant4** para estudiar un sistema completo de detección de neutrones térmicos usando:

- **Moderador de parafina** (fase de termalización)
- **Convertidor delgado** de **grafeno dopado con boro** sobre **Kapton** (fase de captura)
- **Cristales centelladores** (BGO o plástico)
- **Sensores SiPM** (fase de detección óptica)

El objetivo es simular todas las etapas del proceso: termalización, captura, generación de partículas secundarias, centelleo, transporte óptico y detección por SiPM.

Este proyecto complementa los repositorios anteriores de captura y termalización:
https://github.com/luisb0628/neutron-thermalization-geant4  
https://github.com/luisb0628/neutron-capture-boron-graphene

---

## Contenido del repositorio

.
├── CMakeLists.txt
├── main.cc
├── include/                 # Clases del detector y acciones
│   ├── DetectorConstruction.hh
│   ├── PrimaryGeneratorAction.hh
│   ├── RunAction.hh
│   ├── EventAction.hh
│   ├── SteppingAction.hh
│   ├── ScintSD.hh
│   └── OpticalSiPM_SD.hh
├── src/
│   ├── DetectorConstruction.cc
│   ├── PrimaryGeneratorAction.cc
│   ├── RunAction.cc
│   ├── EventAction.cc
│   ├── SteppingAction.cc
│   ├── ScintSD.cc
│   └── OpticalSiPM_SD.cc
├── macros/
├── materials/
└── README.md

---

## Requisitos

- Geant4 ≥ 10.7 (ideal 11.3+) con física HP y Optical Physics  
- CMake ≥ 3.10  
- g++ / clang  
- ROOT (opcional)  
- Python 3 con numpy, uproot, matplotlib, pandas (opcional)

---

## Compilación

mkdir build
cd build
cmake ..
make -j4

---

## Ejecución

./Scintillator_Sipm macros/run.mac  
./Scintillator_Sipm macros/vis.mac  

Parámetros:
- /run/beamOn N
- Energía de neutrones
- Tipo de centellador
- Activación de óptica

---

## Componentes del código

DetectorConstruction → Geometría del moderador, grafeno, Kapton, centellador y SiPM  
PrimaryGeneratorAction → Fuente de neutrones térmicos  
Sensitive Detectors → ScintSD y OpticalSiPM_SD  
Run / Event / Stepping Actions → Registro de energía, fotones y partículas secundarias  

---

## Resultados generados

- Espectros de energía en el centellador  
- Conteo de fotones en el SiPM  
- Distribuciones espaciales  
- Espectros α, Li-7, γ 478 keV en grafeno dopado con boro  

---

## Trabajo futuro

- PDE realista del SiPM  
- Crosstalk y afterpulsing  
- Variación de dopaje  
- Integración con kits CAEN  

---

## Autor

Luis Ricardo Beltrán García  
Maestría en Ingeniería Física – UAN  
lbeltran151@uan.edu.co

---
