# Centellador + SiPM para Detección de Partículas Secundarias (Geant4)

Simulación Monte Carlo en **Geant4** de un sistema de detección que acopla un **cristal centellador** a un **SiPM**, diseñado para detectar las partículas secundarias producidas por la captura de neutrones en grafeno dopado con boro.

Este proyecto es la **tercera etapa** de una cadena de simulación:

1. **Neutron_Thermalization** — genera `AmBePhaseSpace.root`
2. **Film_graphene** — genera `output.root` (ntuples `ProductosCaptura` y `ParticulasTransmitidas`), corre en multinúcleo (MT)
3. **Scintillator_Sipm** (este repo) — detecta las partículas secundarias en el centellador

---

## Modos de operación

| Macro | Fuente | Uso |
|-------|--------|-----|
| `macros/run.mac` | Configurable: pistola estándar, fuentes preset (Co60/Sr90), o phase space (`output.root` de Film_graphene) | General |

---

## Contenido del repositorio

```
.
├── CMakeLists.txt
├── main.cc
├── include/
│   ├── DetectorConstruction.hh   # Geometría centellador + SiPM; 4 tipos de centellador
│   ├── PrimaryGeneratorAction.hh # Lee output.root (ProductosCaptura/ParticulasTransmitidas) o usa pistola estándar
│   ├── ScintSD.hh                # Detector sensible del centellador
│   ├── OpticalSiPM_SD.hh         # Detector óptico del SiPM
│   ├── RunAction.hh
│   ├── EventAction.hh
│   └── ActionInitialization.hh
├── src/
└── macros/
    ├── run.mac                   # Macro general
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
- **ROOT** (Core, RIO, Tree) — para leer `output.root` de Film_graphene en `PrimaryGeneratorAction`
- **CMake** ≥ 3.16, **C++17**
- **Python 3** con: `uproot`, `numpy`, `pandas`, `matplotlib` (opcional)
- `output.root` generado por Film_graphene (para la fuente de phase space, opción A en `run.mac`)

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

Comenta la línea `/gun/inputFile ...` en `macros/run.mac` y descomenta la Opción B o C (pistola estándar / fuente preset), luego:

```bash
cd build
./Scintillator_Sipm ../macros/run.mac
```

Dispara gammas de 662 keV (Cs-137) directamente al centellador.

### Cadena completa (con Film_graphene)

Primero genera `output.root` con Film_graphene (`./Film_graphene macros/run.mac`, en `neutron-capture-graphene-boron-geant4/build/`). `macros/run.mac` de este proyecto ya apunta directo a esa ruta (`../../neutron-capture-graphene-boron-geant4/build/output.root`) — no hace falta copiarlo. Si en vez de eso usaste `python/run_3configs.py` (que genera `output_<config>.root`), ajusta `/gun/inputFile` a ese nombre.

```bash
cd build
./Scintillator_Sipm ../macros/run.mac
```

Lee todas las partículas que llegaron al detector de grafeno (alfa, Li-7, gamma 478 keV, neutrones dispersados, etc. — ntuple `ParticulasTransmitidas`) o solo las nacidas en el grafeno (ntuple `ProductosCaptura`, seleccionable con `/gun/ntuple`) y las simula en el centellador.

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
/gun/inputFile ../../neutron-capture-graphene-boron-geant4/build/output.root
/gun/ntuple ParticulasTransmitidas

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
| `PrimaryGeneratorAction` | Pistola estándar o lectura de `output.root` (ProductosCaptura/ParticulasTransmitidas) |
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
