cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(Scintillator_Sipm)

# --- Encontrar Geant4 ---
find_package(Geant4 REQUIRED ui_all vis_all analysis)

# --- Encontrar ROOT (para leer output.root de Film_graphene en PrimaryGeneratorAction) ---
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree)

# --- Incluir directorios de encabezados ---
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

# --- Archivos fuente ---
set(SOURCES
    src/ActionInitialization.cc
    src/DetectorConstruction.cc
    src/EventAction.cc
    src/PrimaryGeneratorAction.cc
    src/RunAction.cc
    src/ScintSD.cc
    src/OpticalSiPM_SD.cc
)

# --- Ejecutable principal ---
add_executable(Scintillator_Sipm main.cc ${SOURCES})

# --- Enlazar librerías de Geant4 + ROOT ---
target_link_libraries(Scintillator_Sipm ${Geant4_LIBRARIES} ROOT::Core ROOT::RIO ROOT::Tree)
target_include_directories(Scintillator_Sipm PRIVATE ${ROOT_INCLUDE_DIRS})

# --- Copiar directorio de macros al build ---
file(COPY ${PROJECT_SOURCE_DIR}/macros DESTINATION ${PROJECT_BINARY_DIR})

# --- Mensajes informativos ---
message(STATUS "Geant4 found at: ${Geant4_DIR}")
message(STATUS "Project built in: ${PROJECT_BINARY_DIR}")

# --- Opcional: instalación ---
install(TARGETS Scintillator_Sipm DESTINATION bin)

