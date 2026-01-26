# CMakeLists.txt - Configuration de build pour Kitty Chat C++
# Client de chat Matrix avec interface Dear ImGui
cmake_minimum_required(VERSION 3.16)
project(KittyChat VERSION 1.0.0 LANGUAGES CXX)

# Configuration C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options de compilation Windows
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Téléchargement automatique des dépendances avec FetchContent
include(FetchContent)

# Dear ImGui - Bibliothèque d'interface graphique légère
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)
FetchContent_MakeAvailable(imgui)

# nlohmann/json - Bibliothèque JSON pour C++
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# cpp-httplib - Bibliothèque HTTP légère (header-only)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(httplib)

# Création de la bibliothèque ImGui avec backend Win32/DirectX11
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Lien avec les bibliothèques Windows pour DirectX11
target_link_libraries(imgui_lib PUBLIC
    d3d11
    d3dcompiler
    dxgi
)

# Fichiers source de l'application
set(SOURCES
    src/main.cpp
    src/matrix_client.cpp
    src/chat_window.cpp
    src/texture_manager.cpp
)

set(HEADERS
    src/matrix_client.h
    src/chat_window.h
    src/texture_manager.h
    src/stb_image.h
)

# Création de l'exécutable Windows (pas de console)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Inclusion des headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${httplib_SOURCE_DIR}
)

# Lien avec les bibliothèques
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui_lib
    nlohmann_json::nlohmann_json
    ws2_32      # Winsock pour les requêtes réseau
    winhttp     # WinHTTP pour HTTPS
    crypt32     # Cryptographie Windows
)

# Copie des assets dans le dossier de build (si le dossier existe)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
else()
    # Créer le dossier assets vide si il n'existe pas
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/assets)
    message(STATUS "Dossier assets créé (vide)")
endif()

# Message d'information
message(STATUS "Configuration de Kitty Chat C++ terminée")
message(STATUS "  - ImGui: ${imgui_SOURCE_DIR}")
message(STATUS "  - JSON: ${json_SOURCE_DIR}")
message(STATUS "  - HTTPLib: ${httplib_SOURCE_DIR}")


