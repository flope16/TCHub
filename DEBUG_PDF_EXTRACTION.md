# Problème d'extraction PDF - Diagnostic

## Symptôme
L'extracteur tombe sur le fichier `.txt` manuel au lieu d'extraire directement le PDF.

## Ordre des tentatives d'extraction

L'extracteur essaie dans cet ordre :

1. **pdftotext (ligne de commande)** - Cherche dans :
   - `C:\Dev\vcpkg\installed\x64-windows\tools\poppler\pdftotext.exe`
   - `C:\Dev\vcpkg\installed\x64-windows\bin\pdftotext.exe`
   - Le PATH système
   - Le dossier de l'application

2. **API Poppler C++** (si USE_POPPLER est défini)

3. **Fichier .txt existant** (fallback - VOTRE FICHIER MANUEL)

## Solution

### Option A : Installer pdftotext via vcpkg

```powershell
cd C:\Dev\vcpkg
.\vcpkg install poppler:x64-windows --recurse
```

Vérifier l'installation :
```powershell
dir C:\Dev\vcpkg\installed\x64-windows\tools\poppler\pdftotext.exe
```

Si le fichier existe, l'extraction devrait fonctionner automatiquement.

### Option B : Supprimer les fichiers .txt manuels (temporaire)

Pour forcer l'utilisation de pdftotext ou Poppler API :
```powershell
# Supprimer les .txt dans le dossier des PDFs
del "C:\Users\f.laugier\ConvertLindab\*.txt"
```

**Attention** : Ne faites ceci QUE si vous voulez tester l'extraction automatique.

### Option C : Vérifier que Poppler API fonctionne

Le projet a déjà `USE_POPPLER` défini, donc l'API C++ devrait fonctionner.
Mais pdftotext en ligne de commande est souvent plus robuste.

## Logs de diagnostic

Dans la sortie de débogage, vous devriez voir l'un de ces messages :
- `[PopplerPdfExtractor] Extraction reussie avec pdftotext (...)`
- `[PopplerPdfExtractor] Extraction reussie avec API Poppler C++`
- `[PopplerPdfExtractor] Extraction depuis fichier .txt existant` ← **VOUS VOYEZ ÇA**
- `[PopplerPdfExtractor] ECHEC: Aucune methode n'a reussi a extraire du texte`

## Test rapide

1. Vérifiez si pdftotext existe :
   ```powershell
   dir "C:\Dev\vcpkg\installed\x64-windows\tools\poppler\pdftotext.exe"
   ```

2. Si non, installez-le :
   ```powershell
   cd C:\Dev\vcpkg
   .\vcpkg install poppler:x64-windows --recurse
   ```

3. Recompilez et testez

4. Vérifiez les logs pour voir quelle méthode est utilisée
