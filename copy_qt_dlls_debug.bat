@echo off
echo Copie des DLL Qt vers le repertoire Debug...

REM Copier les DLL Qt principales
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Cored.dll" "x64\Debug\"
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Guid.dll" "x64\Debug\"
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Widgetsd.dll" "x64\Debug\"

REM Créer le dossier platforms si nécessaire
if not exist "x64\Debug\platforms\" mkdir "x64\Debug\platforms\"

REM Copier le plugin de plateforme Windows
xcopy /Y /D "C:\Qt\6.10.1\msvc2022_64\plugins\platforms\qwindowsd.dll" "x64\Debug\platforms\"

echo.
echo Copie terminee !
echo Vous pouvez maintenant lancer TCHub.exe
pause
