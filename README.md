# Simulación de Captura de Neutrones en Grafeno Dopado con Boro y Kapton (Geant4)

Simulación Monte Carlo de la **captura de neutrones térmicos** en una estructura de **grafeno dopado con boro (B)** con soporte de **Kapton** usando Geant4.  
El objetivo es investigar la eficiencia de captura de neutrones térmicos por el material dopado, la producción de partículas secundarias, y la respuesta del sistema de soporte/adjunto (Kapton) para aplicaciones en detección de neutrones o sensores avanzados.

---

## Contenido del repositorio

```
.
├── CMakeLists.txt
├── main.cc
├── include/           # Archivos de cabecera (DetectorConstruction, PrimaryGeneratorAction, RunAction, EventAction.)
├── src/               # Código fuente principal
└── README.md
```

---

## ⚙️ Requisitos

- **Geant4** (versión ≥ 10.7 o la que uses) con física de neutrones de alta precisión (HP).  
- **CMake** (≥ 3.10).  
- Compilador C++ (por ejemplo g++ o clang).  
- **ROOT** (opcional, si guardas histogramas/árboles).  
- **Python 3** (opcional) con `pandas`, `matplotlib` y `jupyter`.

> Antes de compilar, asegúrate de cargar el entorno de Geant4 usando algo como:  
> `source ../geant4/bin/geant4.sh`

---

## Compilación

Desde la carpeta raíz del proyecto:

```bash
mkdir -p build
cd build
cmake .. 
make 
```
---

## Ejecución

El proyecto incluye macros de ejemplo dentro de `macros/`. Por ejemplo:

```bash
# Ejecución básica
./neutron-capture-geant4 macros/run.mac

# Ejecución en modo visual (si está habilitado)
./neutron-capture-geant4 macros/vis.mac

```

### Parámetros configurables
- `/run/beamOn <N>`: número de eventos neutrones simulados.
- Energía inicial del haz (neutrones térmicos, e.g., ~0.025 eV, o ligeramente por encima).
- Material: grafeno dopado con boro, soporte de Kapton (espesor, tamaño).
- Posición y dirección del haz.
- Formato de salida (ASCII, CSV, ROOT).

---


## 🧱 Estructura del código

- `DetectorConstruction` → Define geometría: capa de grafeno‑boro, soporte Kapton, fuente de neutrones.  
- `PrimaryGeneratorAction` → Fuente de neutrones térmicos, distribución angular, energía.  
- `RunAction`, `EventAction`, `SteppingAction` → Registro de eventos, capturas, partículas secundarias, ubicación.  
- `macros/run.mac`, `macros/vis.mac` → Parámetros de ejecución, visualización.

---



## 📧 Contacto

Autor: **Luis Beltrán**  
