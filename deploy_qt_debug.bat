@echo off
echo ====================================
echo  TCHub - Copie automatique des DLL Qt
echo ====================================
echo.

cd /d "C:\Dev\TCHub\x64\Debug"

echo Nettoyage des anciennes DLL Qt...
del Qt6*.dll 2>nul
rmdir /s /q platforms 2>nul

echo.
echo Utilisation de windeployqt pour copier toutes les DLL necessaires...
"C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe" --debug TCHub.exe

echo.
echo ====================================
echo  Termine ! Vous pouvez lancer TCHub.exe
echo ====================================
pause
