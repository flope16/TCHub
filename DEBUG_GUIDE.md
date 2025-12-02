# Guide de Debug - TCHub PDF Parser

## 🔍 Voir les logs de debug

Le parser Lindab affiche maintenant des informations de debug détaillées via `OutputDebugString`.

### Méthode 1 : Visual Studio (Recommandé)

Si tu lances TCHub depuis Visual Studio (F5 ou Ctrl+F5) :
- Les messages de debug apparaissent dans la fenêtre **Output** (Sortie)
- Onglet : **Debug** → **Windows** → **Output**

### Méthode 2 : DebugView (Sysinternals)

Si tu lances TCHub.exe directement :

1. **Télécharger DebugView** :
   https://learn.microsoft.com/en-us/sysinternals/downloads/debugview

2. **Lancer DebugView** (en administrateur)

3. **Activer** :
   - Capture → Capture Global Win32
   - Capture → Capture Events

4. **Lancer TCHub.exe** et faire un parsing

5. **Voir les logs** dans DebugView :
   ```
   === DEBUG EXTRACTION PDF ===
   Poppler disponible: OUI
   Texte extrait (1234 caracteres):
   [contenu du PDF...]
   === FIN DEBUG ===

   Ligne avec PCE #1: 1 224931 SR 200 3000 GALV Conduit... 10,00 PCE 18,90 188,99
   Ligne avec PCE #2: 2 225187 ...

   === RESUME PARSING ===
   Lignes totales: 150
   Lignes avec PCE: 25
   Produits extraits: 25
   ===================
   ```

## 📦 Copier les DLLs Poppler

Pour que Poppler fonctionne, exécute ce script PowerShell :

```powershell
.\copy_poppler_dlls_debug.ps1
```

Cela copiera automatiquement :
- `poppler-cpp.dll` (bibliothèque principale)
- `poppler.dll`, `freetype.dll`, `zlib1.dll`, etc. (dépendances)

Vers : `C:\Dev\TCHub\x64\Debug\`

## 🐛 Problèmes courants

### "Aucune donnée extraite"

**Causes possibles :**
1. **Poppler non disponible** → Vérifier dans les logs : `Poppler disponible: NON`
   - Solution : Exécuter `copy_poppler_dlls_debug.ps1`

2. **Texte extrait vide** → Vérifier : `Texte extrait (0 caracteres)`
   - Solution : Le PDF est peut-être protégé ou corrompu

3. **Aucune ligne avec PCE** → Vérifier : `Lignes avec PCE: 0`
   - Solution : Le format du PDF ne correspond pas à Lindab
   - Créer un fichier `.txt` avec le même nom que le PDF

4. **Regex ne match pas** → Vérifier : `Lignes avec PCE: 25` mais `Produits extraits: 0`
   - Solution : Ajuster le regex dans `LindabPdfParser.cpp`

### Format Lindab attendu

Le parser cherche des lignes avec ce format :
```
N° Référence Désignation Qté PCE Prix Montant
1  224931    SR 200...   10,00 PCE 18,90 188,99
```

**Éléments requis :**
- Mot-clé `PCE` sur la ligne
- Référence à 6 chiffres
- Quantité, Prix, Montant (format `12,34` ou `12.34`)
