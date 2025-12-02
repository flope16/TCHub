@echo off
echo ============================================
echo  TCHub - Copie complete des DLL (Qt + Poppler)
echo ============================================
echo.

REM Configuration des chemins
set QT_PATH=C:\Qt\6.10.1\msvc2022_64
set VCPKG_PATH=C:\Dev\vcpkg\installed\x64-windows
set OUTPUT_DIR=C:\Dev\TCHub\x64\Debug

echo Repertoire de sortie : %OUTPUT_DIR%
cd /d "%OUTPUT_DIR%"

echo.
echo ============================================
echo  ETAPE 1 : Copie des DLL Qt
echo ============================================

REM Utiliser windeployqt pour copier automatiquement toutes les DLL Qt
"%QT_PATH%\bin\windeployqt.exe" --debug --no-translations TCHub.exe

echo.
echo ============================================
echo  ETAPE 2 : Copie des DLL Poppler
echo ============================================

REM Copier poppler-cpp.dll
if exist "%VCPKG_PATH%\bin\poppler-cpp.dll" (
    echo [COPIE] poppler-cpp.dll
    xcopy /Y "%VCPKG_PATH%\bin\poppler-cpp.dll" .
) else (
    echo [ERREUR] poppler-cpp.dll introuvable dans vcpkg!
    echo Chemin recherche: %VCPKG_PATH%\bin\poppler-cpp.dll
)

REM Copier toutes les dependances Poppler (freetype, zlib, jpeg, png, etc.)
echo [COPIE] Dependances Poppler...
for %%F in (
    freetype.dll
    zlib1.dll
    jpeg62.dll
    libpng16.dll
    openjp2.dll
    tiff.dll
    lzma.dll
    bz2.dll
    lcms2.dll
    poppler.dll
) do (
    if exist "%VCPKG_PATH%\bin\%%F" (
        echo   - %%F
        xcopy /Y "%VCPKG_PATH%\bin\%%F" . >nul 2>&1
    )
)

echo.
echo ============================================
echo  ETAPE 3 : Verification
echo ============================================

echo Fichiers DLL presents :
dir /B *.dll

echo.
echo ============================================
echo  TERMINE !
echo ============================================
echo.
echo Appuyez sur une touche pour lancer TCHub.exe...
pause > nul
start TCHub.exe
