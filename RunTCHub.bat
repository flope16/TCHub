@echo off
REM Script de lancement de TCHub avec les chemins Qt et vcpkg
REM Utilisez ce script si vous lancez TCHub.exe en dehors de Visual Studio

SET PATH=C:\Qt\6.10.1\msvc2022_64\bin;C:\Dev\vcpkg\installed\x64-windows\bin;%PATH%

REM Lancer l'application (ajustez le chemin selon votre configuration)
IF EXIST "x64\Debug\TCHub.exe" (
    echo Lancement de TCHub.exe (Debug)...
    start "" "x64\Debug\TCHub.exe"
) ELSE IF EXIST "x64\Release\TCHub.exe" (
    echo Lancement de TCHub.exe (Release)...
    start "" "x64\Release\TCHub.exe"
) ELSE (
    echo ERREUR: TCHub.exe introuvable. Veuillez compiler le projet d'abord.
    pause
)
