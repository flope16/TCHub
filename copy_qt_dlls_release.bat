@echo off
echo Copie des DLL Qt vers le repertoire Release...

REM Copier les DLL Qt principales
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Core.dll" "x64\Release\"
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Gui.dll" "x64\Release\"
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Widgets.dll" "x64\Release\"

REM Créer le dossier platforms si nécessaire
if not exist "x64\Release\platforms\" mkdir "x64\Release\platforms\"

REM Copier le plugin de plateforme Windows
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\plugins\platforms\qwindows.dll" "x64\Release\platforms\"

echo.
echo Copie terminee !
echo Vous pouvez maintenant lancer TCHub.exe
pause
