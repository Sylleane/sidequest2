@echo off
REM ============================================
REM Script de lancement pour Kitty Chat C++
REM Ce script compile et lance l'application
REM ============================================
setlocal enabledelayedexpansion

echo ========================================
echo   Lanceur Kitty Chat C++
echo ========================================
echo.

REM Aller a la racine du projet (parent de cicd/)
cd /d "%~dp0.."

REM Recherche de CMake - d'abord dans le PATH, puis dans Visual Studio
set "CMAKE_CMD=cmake"
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    REM CMake pas dans le PATH, chercher dans Visual Studio
    set "VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if exist "!VS_CMAKE!" (
        set "CMAKE_CMD=!VS_CMAKE!"
        echo [INFO] Utilisation de CMake de Visual Studio
    ) else (
        set "VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if exist "!VS_CMAKE!" (
            set "CMAKE_CMD=!VS_CMAKE!"
            echo [INFO] Utilisation de CMake de Visual Studio Pro
        ) else (
            set "VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if exist "!VS_CMAKE!" (
                set "CMAKE_CMD=!VS_CMAKE!"
                echo [INFO] Utilisation de CMake de Visual Studio Enterprise
            ) else (
                echo [ERREUR] CMake n'est pas installe.
                echo.
                echo Options :
                echo 1. Installez CMake depuis https://cmake.org/download/
                echo 2. Installez Visual Studio avec les composants C++ CMake
                echo.
                pause
                exit /b 1
            )
        )
    )
)

echo [OK] CMake trouve
"!CMAKE_CMD!" --version | findstr /C:"version"
echo.

REM Verifier si l'executable existe deja
if exist "build\Release\KittyChat.exe" (
    echo [INFO] Executable trouve, lancement direct...
    echo.
    goto :launch
)

if exist "build\Debug\KittyChat.exe" (
    echo [INFO] Executable Debug trouve, lancement direct...
    echo.
    goto :launch_debug
)

REM Creation du dossier assets si necessaire
if not exist "assets" (
    echo [INFO] Creation du dossier assets...
    mkdir assets
    echo. > assets\.gitkeep
)

REM Creation du dossier de build si necessaire
if not exist "build" (
    echo [INFO] Creation du dossier de build...
    mkdir build
)

cd build

REM Configuration avec CMake
echo ========================================
echo Configuration du projet avec CMake...
echo ========================================
echo.
echo Cette etape peut prendre quelques minutes
echo (telechargement des dependances)
echo.

"!CMAKE_CMD!" .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] La configuration CMake a echoue.
    echo.
    echo Verifiez que Visual Studio est installe avec les composants C++.
    echo.
    cd ..
    pause
    exit /b 1
)

echo.
echo [OK] Configuration terminee.
echo.

REM Compilation
echo ========================================
echo Compilation du projet...
echo ========================================
echo.

"!CMAKE_CMD!" --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] La compilation a echoue.
    echo.
    echo Consultez les messages d'erreur ci-dessus.
    echo.
    cd ..
    pause
    exit /b 1
)

echo.
echo [OK] Compilation terminee.
echo.

cd ..

:launch
echo ========================================
echo Demarrage de Kitty Chat...
echo ========================================
echo.
start "" "build\Release\KittyChat.exe"
goto :end

:launch_debug
echo ========================================
echo Demarrage de Kitty Chat (Debug)...
echo ========================================
echo.
start "" "build\Debug\KittyChat.exe"
goto :end

:end
echo [INFO] Application lancee.
echo.
echo Vous pouvez fermer cette fenetre.
echo.
timeout /t 5
exit /b 0
