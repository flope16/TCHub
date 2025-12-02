# Télécharger pdftotext.exe pour Windows

## Option A : Binaires pré-compilés (RECOMMANDÉ)

1. **Télécharger** : https://github.com/oschwartz10612/poppler-windows/releases/
   - Prendre la dernière version (ex: Release-24.08.0-0)
   - Télécharger `Release-24.08.0-0.zip`

2. **Extraire** le ZIP

3. **Copier** `Library/bin/pdftotext.exe` vers :
   ```
   C:\Dev\vcpkg\installed\x64-windows\tools\poppler\pdftotext.exe
   ```

   OU dans le dossier du projet :
   ```
   C:\path\to\TCHub\pdftotext.exe
   ```

4. **Mettre à jour** `PopplerPdfExtractor.cpp` (ligne 82-86) avec le nouveau chemin

## Option B : Ajouter au PATH

1. Créer un dossier : `C:\poppler-utils\`
2. Y placer `pdftotext.exe` et toutes les DLLs nécessaires
3. Ajouter `C:\poppler-utils\` au PATH Windows

## Vérification

Dans PowerShell :
```powershell
pdftotext -v
```

Devrait afficher la version de Poppler.
