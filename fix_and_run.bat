@echo off
echo ============================================
echo  TCHub - Verification et copie des DLL Qt
echo ============================================
echo.

cd /d "C:\Dev\TCHub\x64\Debug"

echo Verification des DLL Qt...
if exist "Qt6Cored.dll" (
    echo [OK] Qt6Cored.dll trouvee
) else (
    echo [MANQUANT] Qt6Cored.dll - copie en cours...
    xcopy /Y "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Cored.dll" .
)

if exist "Qt6Guid.dll" (
    echo [OK] Qt6Guid.dll trouvee
) else (
    echo [MANQUANT] Qt6Guid.dll - copie en cours...
    xcopy /Y "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Guid.dll" .
)

if exist "Qt6Widgetsd.dll" (
    echo [OK] Qt6Widgetsd.dll trouvee
) else (
    echo [MANQUANT] Qt6Widgetsd.dll - copie en cours...
    xcopy /Y "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Widgetsd.dll" .
)

if not exist "platforms\" (
    echo [CREATION] Dossier platforms...
    mkdir platforms
)

if exist "platforms\qwindowsd.dll" (
    echo [OK] platforms\qwindowsd.dll trouvee
) else (
    echo [MANQUANT] platforms\qwindowsd.dll - copie en cours...
    xcopy /Y "C:\Qt\6.10.1\msvc2022_64\plugins\platforms\qwindowsd.dll" platforms\
)

echo.
echo ============================================
echo Utilisation de windeployqt pour garantir toutes les dependances...
echo ============================================
"C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" --debug --no-translations TCHub.exe

echo.
echo ============================================
echo  TERMINE !
echo  Vous pouvez maintenant lancer TCHub.exe
echo ============================================
echo.
echo Appuyez sur une touche pour lancer TCHub.exe...
pause > nul
start TCHub.exe
