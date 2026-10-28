# Cristal centelleador con SiPM (Geant4)

Simulación Monte Carlo en **Geant4** de la respuesta de un **cristal centelleador acoplado a un SiPM** a las partículas que salen de una película de grafeno dopado con boro. El programa transporta el centelleo como fotones ópticos hasta el SiPM, cuenta los fotoelectrones de cada evento y los atribuye a la partícula primaria que los originó.

Es la **tercera etapa** de una cadena de tres simulaciones:

| Etapa | Repositorio | Entrada | Salida |
|---|---|---|---|
| 1. Termalización | [neutron-thermalization-geant4](https://github.com/luisb0628/neutron-thermalization-geant4) | — | `AmBePhaseSpace.root`, `Cs137PhaseSpace.root` |
| 2. Captura | [neutron-capture-graphene-boron-geant4](https://github.com/luisb0628/neutron-capture-graphene-boron-geant4) | phase space de la etapa 1 | `output.root` |
| 3. Centelleador | **Scintillator_Sipm** (este) | `output.root` de la etapa 2 | `output.root` |

---

## Física y geometría

**Lista de física:** `QGSP_BERT_HP` (la misma de las tres etapas) más `G4OpticalPhysics`, para el centelleo y el transporte de los fotones ópticos (absorción, reflexión y refracción en las superficies).

**Cristal.** BGO de (6 × 6 × 15) mm³, con la cara frontal en *z* = 0, dentro de un mundo de aire de 20 cm de lado. Va envuelto en teflón, modelado como una superficie óptica rugosa con reflectividad de 0,95 en todas las caras menos la del SiPM. Propiedades asignadas al BGO: 8200 fotones por MeV, emisión gaussiana centrada en 2,56 eV (484 nm), tiempo de decaimiento de 300 ns, índice de refracción de 2,15, longitud de absorción de 25 cm y constante de Birks de 0,012 mm/MeV.

**Otros centelleadores** (`/scint/type`): `PLASTIC` (10 000 fotones/MeV, 2,1 ns), `PLASTIC_B` (6500 fotones/MeV, 2,5 ns), `CSI` (54 000 fotones/MeV, 1000 ns) y `LYSO` (32 000 fotones/MeV, 40 ns).

**SiPM.** Capa de silicio de 1 mm del mismo tamaño transversal que el cristal y con su índice de refracción, así que no hay reflexión de Fresnel en la unión. Detecta cada fotón que llega con una probabilidad de 30 % (PDE constante con la longitud de onda), y cada fotón detectado es un fotoelectrón. **No se modelan** el ruido oscuro, la diafonía, el *afterpulsing* ni la saturación de microceldas.

**Primarios.**
- **Cadena completa:** se leen las partículas que salen de la película en la etapa 2. Se agrupan por `EventID`, así que todas las de un mismo evento de la etapa 2 forman un evento aquí, y se recorren de forma cíclica. Cada una nace 10 µm antes de la cara frontal del cristal, con su posición en el plano del detector, su energía y su dirección. Como el cristal mide 6 × 6 mm, solo entran las que salen por la zona central.
- **Calibración:** una pistola estándar de Geant4 (por ejemplo, γ de 0,661657 MeV desde 1 mm delante del cristal) o una fuente predefinida (`/gun/source Cs137 | Co60 | Sr90`).

---

## Requisitos

- **Geant4 11** con física óptica, datos HP, `analysis` y, para visualizar, `ui_all` y `vis_all`.
- **ROOT**, para leer el `output.root` de la etapa 2 en `PrimaryGeneratorAction`.
- **CMake** ≥ 3.16 y un compilador **C++17**.
- **Python 3** con `uproot`, `numpy` y `pandas`, para el análisis.

Las corridas de la tesis se hicieron en Docker con la imagen `luisb28/geant4-clase:11.4-python`, montando la carpeta que contiene los tres repositorios en `/workspace`.

---

## Compilación y ejecución

```bash
source /ruta/a/geant4/bin/geant4.sh
mkdir -p build && cd build
cmake ..
make -j$(nproc)

./Scintillator_Sipm ../macros/run_gf.mac    # cadena completa: película con boro
./Scintillator_Sipm                         # modo interactivo con visualización
```

El programa usa varios hilos y siempre escribe `output.root` en la carpeta donde se ejecuta. Para conservar varias corridas, renombra el archivo después de cada una.

**Calibración con la pistola estándar.** En modo multihilo, los comandos `/gun/particle`, `/gun/energy`, `/gun/position` y `/gun/direction` deben ir **después** de `/run/initialize`; antes de esa línea Geant4 responde `COMMAND NOT FOUND`. Ejemplo:

```
/scint/type BGO
/run/initialize
/gun/particle gamma
/gun/energy 661.657 keV
/gun/position 0 0 -1 mm
/gun/direction 0 0 1
/run/beamOn 100000
```

### Número de hilos

El programa corre en varios hilos y, por defecto, usa todos los núcleos del equipo (`SetNumberOfThreads(G4GetNumberOfCores())` en `main.cc`). Hay dos formas de cambiarlo sin tocar el código:

- **En la macro**, antes de `/run/initialize`. Todas las macros de corrida traen la línea comentada:

  ```
  /run/numberOfThreads 4
  /run/initialize
  ```

- **Con una variable de entorno**, que se impone a lo que digan el código y la macro:

  ```bash
  G4FORCENUMBEROFTHREADS=4 ./Scintillator_Sipm ../macros/run_gf.mac
  # dentro de Docker:
  docker exec -e G4FORCENUMBEROFTHREADS=4 <contenedor> bash -lc "cd ... && ./Scintillator_Sipm ../macros/run_gf.mac"
  ```

Para limitar además **qué** núcleos se usan, en Linux se puede lanzar con `taskset -c 0-3 ./Scintillator_Sipm ...` o crear el contenedor con `--cpuset-cpus="0-3"`. En ese caso hay que fijar también el número de hilos, porque Geant4 cuenta los núcleos de toda la máquina y lanzaría más hilos que núcleos asignados. Si se corren varias simulaciones a la vez, conviene repartir los núcleos entre ellas. Con otro número de hilos cambian las secuencias de números aleatorios: los resultados son estadísticamente equivalentes, pero no idénticos evento a evento.

---

## Macros

| Macro | Entrada (etapa 2) | Caso |
|---|---|---|
| `macros/run_gf.mac` | `output_run_g.root` | primarios del AmBe, Kapton + grafeno con boro |
| `macros/run_ngf.mac` | `output_run.root` | primarios del AmBe, sin grafeno |
| `macros/run_cs137_gf.mac` | `output_run_cs137_g.root` | primarios del ¹³⁷Cs, Kapton + grafeno con boro |
| `macros/run_cs137_ngf.mac` | `output_run_cs137.root` | primarios del ¹³⁷Cs, sin grafeno |
| `macros/run.mac` | configurable | macro general comentada (phase space, pistola o fuente predefinida) |
| `macros/vis1.mac` | — | visualización |

Las macros de la cadena usan un cristal de BGO y 2×10⁶ eventos, y leen los archivos de `../../neutron-capture-graphene-boron-geant4/build/`.

### Comandos

| Comando | Descripción |
|---|---|
| `/scint/type BGO \| PLASTIC \| PLASTIC_B \| CSI \| LYSO` | Material del cristal (antes de `/run/initialize`) |
| `/gun/inputFile <ruta>` | `output.root` de la etapa 2 que alimenta los primarios |
| `/gun/ntuple ParticulasTransmitidas \| ProductosCaptura` | Tabla que se lee del archivo (por defecto, `ParticulasTransmitidas`) |
| `/gun/source Cs137 \| Co60 \| Sr90` | Fuente de calibración predefinida (Co-60: γ de 1,1732 y 1,3325 MeV en cascada; Sr-90: espectro β de Sr-90/Y-90) |

---

## Salida (`output.root`)

| Tabla | Contenido |
|---|---|
| `ScintEvent` | Por evento: energía depositada en el cristal (`TotalEdep_keV`) y número de fotones ópticos producidos (`nOptPhotons`). |
| `SiPMSummary` | Por evento: fotoelectrones totales (`nPE_total`) y su reparto según la primaria que los originó (`nPE_alpha`, `nPE_Li7`, `nPE_gamma`, `nPE_electron`, `nPE_neutron`, `nPE_proton`, `nPE_other`). La atribución sube por el árbol de parentesco: un γ que produce un electrón Compton que centellea cuenta como γ. |
| `SiPMData` | Tiempo (`time_ns`) y energía (`energy_eV`) de cada fotón detectado, con la primaria de origen. Solo se guarda para los primeros 10 000 eventos. |

La calibración en energía se hace aparte, en el análisis: se ajusta una gaussiana al fotopico del ¹³⁷Cs en `nPE_total` y se convierten los fotoelectrones a MeV.

---

## Estructura del código

| Clase | Responsabilidad |
|---|---|
| `DetectorConstruction` | Cristal, envoltura de teflón, SiPM y propiedades ópticas de cada centelleador; comando `/scint/type` |
| `PrimaryGeneratorAction` | Lee el phase space de la etapa 2 (agrupado por evento), o usa la pistola estándar o una fuente predefinida; comandos `/gun/inputFile`, `/gun/ntuple` y `/gun/source` |
| `ScintSD` | Energía depositada y fotones producidos en el cristal |
| `OpticalSiPM_SD` | Detección de fotones en el SiPM con la PDE y atribución a la primaria |
| `RunAction` / `EventAction` | Archivo ROOT, tablas y estado por evento |

---

## Limitaciones

- La PDE del SiPM es constante: no depende de la longitud de onda.
- No hay ruido oscuro, diafonía, *afterpulsing* ni saturación de microceldas, así que el ancho de los picos solo incluye la estadística de los fotoelectrones y la variación de la luz recogida según el punto de interacción.
- El rendimiento luminoso se asigna constante con la energía. La constante de Birks se fija en el material, pero el código no registra un `G4EmSaturation` en `G4Scintillation`, así que no hay que contar con que reduzca la luz.

---

## Autor

**Luis Ricardo Beltrán García** — Maestría en Ingeniería Física, Universidad Antonio Nariño (UAN)
lbeltran151@uan.edu.mx
