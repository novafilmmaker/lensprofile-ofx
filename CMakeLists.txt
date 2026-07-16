cmake_minimum_required(VERSION 3.20)

if(NOT APPLE)
    message(FATAL_ERROR "Este CMakeLists.txt es especifico de macOS/Metal (Fase 1). "
                         "El backend Windows/CUDA es la Fase 8 de la hoja de ruta.")
endif()

project(LensProfileOFX LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Dependencias externas, descargadas automaticamente en la primera
# configuracion de CMake (no hace falta clonar submodulos a mano).
# ---------------------------------------------------------------------------
include(FetchContent)

# OpenFX SDK (headers + "Support Library" en C++ que envuelve la API OFX cruda)
FetchContent_Declare(
    openfx
    GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/openfx.git
    GIT_TAG main
)
FetchContent_GetProperties(openfx)
if(NOT openfx_POPULATED)
    message(STATUS "Descargando OpenFX SDK (primera vez, puede tardar un poco)...")
    FetchContent_Populate(openfx)
endif()
set(OPENFX_PATH ${openfx_SOURCE_DIR})
set(OFX_SUPPORT_HEADER_DIR "${OPENFX_PATH}/Support/include")
set(OFX_SUPPORT_LIBRARY_DIR "${OPENFX_PATH}/Support/Library")

# nlohmann/json (header-only) para leer los perfiles .json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# ---------------------------------------------------------------------------
# Fuentes
# ---------------------------------------------------------------------------

# "Support Library" oficial de OFX -- envuelve la API C cruda en clases C++
# (OFX::ImageEffect, OFX::Clip, etc). La compilamos junto con nuestro propio
# codigo porque el SDK no publica una libreria precompilada.
set(OFX_SUPPORT_SOURCES
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsCore.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsImageEffect.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsInteract.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsLog.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsMultiThread.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsParams.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsProperty.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsPropertyValidation.cpp"
)

set(PLUGIN_SOURCES
    src/ChromaticAberrationPlugin.cpp
    src/ChromaticAberrationProcessor.cpp
    src/ProfileLoader.cpp
    src/BundleResourcePath.cpp
    metal/MetalKernel.mm
)

add_library(LensProfileOFX MODULE ${PLUGIN_SOURCES} ${OFX_SUPPORT_SOURCES})

target_include_directories(LensProfileOFX PRIVATE
    src
    ${OPENFX_PATH}/include
    ${OFX_SUPPORT_HEADER_DIR}
    ${OPENFX_PATH}/Support/Plugins/include
)

target_link_libraries(LensProfileOFX PRIVATE nlohmann_json::nlohmann_json)

# El archivo Metal se compila sin ARC: hacemos casts manuales del puntero de
# cola de comandos que entrega Resolve, y seguimos el mismo patron de
# manejo manual de memoria que usan otros plugins OFX/Metal publicados.
set_source_files_properties(metal/MetalKernel.mm PROPERTIES
    COMPILE_FLAGS "-fno-objc-arc"
)

find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
target_link_libraries(LensProfileOFX PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK})

target_compile_options(LensProfileOFX PRIVATE -fvisibility=hidden)

# ---------------------------------------------------------------------------
# Empaquetado como .ofx.bundle (estructura obligatoria de OFX en macOS)
# ---------------------------------------------------------------------------
set_target_properties(LensProfileOFX PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    OUTPUT_NAME "LensProfileOFX"
    LINK_FLAGS "-fvisibility=hidden -exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/osxSymbols"
)

set(BUNDLE_DIR ${CMAKE_CURRENT_BINARY_DIR}/LensProfileOFX.ofx.bundle)

add_custom_command(TARGET LensProfileOFX POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUNDLE_DIR}/Contents/MacOS
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUNDLE_DIR}/Contents/Resources/profiles
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:LensProfileOFX> ${BUNDLE_DIR}/Contents/MacOS/LensProfileOFX.ofx
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist ${BUNDLE_DIR}/Contents/Info.plist
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/profiles/canon_k35_50mm.json ${BUNDLE_DIR}/Contents/Resources/profiles/canon_k35_50mm.json
    COMMENT "Ensamblando ${BUNDLE_DIR}"
)

message(STATUS "Bundle final se arma en: ${BUNDLE_DIR}")
message(STATUS "Instalar copiando ese bundle a /Library/OFX/Plugins/ (ver install.sh)")
