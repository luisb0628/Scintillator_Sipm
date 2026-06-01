# Centellador + SiPM para Detección de Partículas Secundarias (Geant4)

Simulación Monte Carlo en **Geant4** de un sistema de detección que acopla un **cristal centellador** a un **SiPM**, diseñado para detectar las partículas secundarias producidas por la captura de neutrones en grafeno dopado con boro.

Este proyecto es la **tercera etapa** de una cadena de simulación:

1. **Neutron_Thermalization** — genera `AmBePhaseSpace.root`
2. **Film_graphene** — genera `generated_particles.txt` y `transmitted_particles.txt`
3. **Scintillator_Sipm** (este repo) — detecta las partículas secundarias en el centellador

---

## Modos de operación

| Macro | Fuente | Uso |
|-------|--------|-----|
| `macros/run_calibration.mac` | Pistola estándar (gamma 662 keV, Cs-137) | Calibración del sistema |
| `macros/run_transmitted.mac` | `transmitted_particles.txt` de Film_graphene | Simulación completa de la cadena |
| `macros/run.mac` | Configurable | General |

---

## Contenido del repositorio

```
.
├── CMakeLists.txt
├── main.cc
├── include/
│   ├── DetectorConstruction.hh   # Geometría centellador + SiPM; 4 tipos de centellador
│   ├── PrimaryGeneratorAction.hh # Lee transmitted_particles.txt o usa pistola estándar
│   ├── ScintSD.hh                # Detector sensible del centellador
│   ├── OpticalSiPM_SD.hh         # Detector óptico del SiPM
│   ├── RunAction.hh
│   ├── EventAction.hh
│   └── ActionInitialization.hh
├── src/
└── macros/
    ├── run.mac                   # Macro general
    ├── run_calibration.mac       # Calibración con gamma 662 keV
    ├── run_transmitted.mac       # Fuente: transmitted_particles.txt de Film_graphene
    └── vis1.mac                  # Visualización
```

---

## Tipos de centellador

El tipo se configura con `/scint/type` antes de `/run/initialize`:

| Tipo | Pico emisión | Índice refracción | Yield (fotones/MeV) | τ (ns) |
|------|-------------|-------------------|---------------------|--------|
| `BGO` (default) | 485 nm | 2.15 | 8 200 | 300 |
| `PLASTIC` | 425 nm | 1.58 | 10 000 | 2.1 |
| `CSI` | 550 nm | 1.80 | 54 000 | 1 000 |
| `LYSO` | 440 nm | 1.82 | 32 000 | 40 |

---

## Requisitos

- **Geant4** ≥ 10.7 compilado con `ui_all`, `vis_all`, `analysis` y Optical Physics
- **CMake** ≥ 3.16, **C++17**
- **Python 3** con: `uproot`, `numpy`, `pandas`, `matplotlib` (opcional)
- `transmitted_particles.txt` generado por Film_graphene (para `run_transmitted.mac`)

---

## Compilación

```bash
source /path/to/geant4/install/bin/geant4.sh
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

---

## Ejecución

### Calibración (sin Film_graphene)

```bash
cd build
./Scintillator_Sipm ../macros/run_calibration.mac
```

Dispara gammas de 662 keV (Cs-137) directamente al centellador.

### Cadena completa (con Film_graphene)

Primero genera `transmitted_particles.txt` con Film_graphene, luego:

```bash
cd build
./Scintillator_Sipm ../macros/run_transmitted.mac
```

Lee todas las partículas que llegaron al detector de grafeno (alfa, Li-7, gamma 478 keV, neutrones dispersados, etc.) y las simula en el centellador.

### Modo interactivo

```bash
cd build
./Scintillator_Sipm
```

---

## Comandos del macro

```bash
# Tipo de centellador
/scint/type BGO          # BGO | PLASTIC | CSI | LYSO

# Modo calibración (pistola estándar)
/gun/particle gamma
/gun/energy 662 keV
/gun/position 0 0 -1 cm
/gun/direction 0 0 1

# Modo cadena completa (phase space de Film_graphene)
/gun/transmittedFile ../../Film_graphene/build/transmitted_particles_kaptonfirst.txt

/run/initialize
/run/beamOn 10000
```

---

## Resultados generados

- Espectros de deposición de energía en el centellador
- Conteo de fotones ópticos detectados por el SiPM
- Distribuciones espaciales
- Espectros de α, Li-7 y gamma 478 keV (captura B-10)

---

## Estructura del código

| Clase | Responsabilidad |
|-------|----------------|
| `DetectorConstruction` | Geometría centellador + SiPM; propiedades ópticas por tipo |
| `PrimaryGeneratorAction` | Pistola estándar o lectura de `transmitted_particles.txt` |
| `ScintSD` | Registra deposición de energía y fotones en el centellador |
| `OpticalSiPM_SD` | Detecta fotones ópticos en el SiPM |
| `RunAction` | Gestiona archivos de salida ROOT/CSV |
| `EventAction` | Inicialización por evento |

---

## Trabajo futuro

- PDE realista del SiPM
- Crosstalk y afterpulsing
- Variación del dopaje de boro
- Integración con kits CAEN

---

## Contacto

Proyecto desarrollado por **Luis Ricardo Beltrán García**  
Maestría en Ingeniería Física — UAN  
lbeltran151@uan.edu.mx
