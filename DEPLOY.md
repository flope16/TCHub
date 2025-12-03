# Guide de déploiement de TCHub

## Configuration actuelle : Mode Développement

L'application est actuellement configurée pour **utiliser les DLL Qt directement depuis l'installation Qt** (`C:\Qt\6.10.1\msvc2022_64\bin`).

**Avantages :**
- Pas de copie de DLL lors du build (plus rapide)
- Moins d'espace disque utilisé

**Inconvénients :**
- ⚠️ L'application ne fonctionnera PAS sur une autre machine
- ⚠️ Si vous désinstallez ou mettez à jour Qt, l'application se cassera
- ⚠️ Non distribuable

## Pour déployer l'application sur une autre machine

### Option 1 : Script de déploiement automatique (Recommandé)

```batch
@echo off
echo === Preparation du package de deploiement ===

:: Copier l'executable
xcopy /Y "x64\Release\TCHub.exe" "Deploy\" || goto :error

:: Copier les DLL Qt
xcopy /Y "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Core.dll" "Deploy\" || goto :error
xcopy /Y "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Gui.dll" "Deploy\" || goto :error
xcopy /Y "C:\Qt\6.10.1\msvc2022_64\bin\Qt6Widgets.dll" "Deploy\" || goto :error

:: Copier les plugins Qt
mkdir "Deploy\platforms" 2>nul
xcopy /Y "C:\Qt\6.10.1\msvc2022_64\plugins\platforms\qwindows.dll" "Deploy\platforms\" || goto :error

:: Copier les DLL vcpkg (Poppler, zlib, etc.)
xcopy /Y "C:\Dev\vcpkg\installed\x64-windows\bin\poppler-cpp.dll" "Deploy\" 2>nul
xcopy /Y "C:\Dev\vcpkg\installed\x64-windows\bin\poppler.dll" "Deploy\" 2>nul
xcopy /Y "C:\Dev\vcpkg\installed\x64-windows\bin\zlib1.dll" "Deploy\" 2>nul
xcopy /Y "C:\Dev\vcpkg\installed\x64-windows\bin\freetype.dll" "Deploy\" 2>nul
:: ... (ajouter toutes les autres DLL vcpkg nécessaires)

:: Copier le qt.conf pour déploiement
xcopy /Y "qt.conf.deploy" "Deploy\qt.conf*" || goto :error

echo === Package de deploiement cree avec succes dans le dossier Deploy ===
goto :end

:error
echo === ERREUR lors de la creation du package ===
exit /b 1

:end
```

### Option 2 : Utiliser windeployqt (Outil officiel Qt)

```batch
cd x64\Release
C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe TCHub.exe --release --no-translations
:: Puis copier manuellement les DLL vcpkg
```

### Option 3 : Retour au mode copie automatique

Si vous préférez que les DLL soient automatiquement copiées lors du build (comme avant), supprimez les modifications récentes et utilisez le commit précédent.

## Fichiers de configuration

### `qt.conf` (Mode développement - actuel)
```ini
[Paths]
Prefix = C:/Qt/6.10.1/msvc2022_64
Plugins = C:/Qt/6.10.1/msvc2022_64/plugins
Libraries = C:/Qt/6.10.1/msvc2022_64/bin
```
☝️ Pointe vers l'installation Qt système

### `qt.conf.deploy` (Mode déploiement)
```ini
[Paths]
Prefix = .
Plugins = plugins
```
☝️ Utilise les DLL locales (à côté de l'EXE)

## Recommandation

Pour le **développement quotidien** : Mode actuel (pointant vers Qt) ✅
Pour la **distribution** : Utilisez le script de déploiement ou windeployqt ✅
