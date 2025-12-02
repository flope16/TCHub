# Solutions d'extraction PDF pour Lindab

## 🎯 Problème
Le package vcpkg `poppler:x64-windows` installe **uniquement l'API C++**, pas les utilitaires en ligne de commande comme `pdftotext.exe`.

## ✅ 3 Solutions disponibles (par ordre de priorité)

### Solution 1 : Télécharger pdftotext.exe (RECOMMANDÉ)

**Avantages :** Extraction la plus robuste, préserve parfaitement le layout

**Étapes :**

1. **Télécharger** les binaires Poppler pour Windows :
   - https://github.com/oschwartz10612/poppler-windows/releases/
   - Prendre la dernière version (ex: Release-24.08.0-0)

2. **Extraire** le fichier `pdftotext.exe` depuis `Library/bin/`

3. **Placer** dans un des emplacements suivants :
   ```
   Option A : C:\Dev\vcpkg\installed\x64-windows\tools\poppler\pdftotext.exe
   Option B : Dans le dossier TCHub\x64\Debug\pdftotext.exe (à côté de TCHub.exe)
   Option C : C:\poppler-utils\pdftotext.exe (et ajouter au PATH)
   ```

4. **Copier aussi les DLLs** nécessaires :
   - Toutes les DLLs du dossier `Library/bin/` du ZIP Poppler
   - Les placer au même endroit que `pdftotext.exe`

**Code mis à jour :**
Le code essaie maintenant automatiquement ces emplacements :
- `pdftotext` (PATH)
- `.\pdftotext.exe` (dossier TCHub.exe) ← PLUS SIMPLE
- `C:\Dev\vcpkg\installed\x64-windows\tools\poppler\pdftotext.exe`
- `C:\poppler-utils\pdftotext.exe`

---

### Solution 2 : Utiliser l'API Poppler C++ améliorée

**Avantages :** Déjà installé, pas de dépendances externes

**Ce qui a été amélioré :**
```cpp
// Avant : extraction simple sans layout
poppler::ustring text = page->text();

// Maintenant : extraction avec rectangle de page (meilleur layout)
poppler::rectf pageRect(0, 0, page->page_rect().width(), page->page_rect().height());
poppler::ustring text = page->text(pageRect);
```

**Quand ça fonctionne :**
- PDFs avec texte simple et bien structuré
- PDFs créés directement (pas scannés)

**Limitations potentielles :**
- Certains PDFs complexes peuvent perdre la mise en page
- Les tableaux peuvent être mal extraits

---

### Solution 3 : Export manuel en .txt (Temporaire)

**Quand l'utiliser :** Pour tester rapidement le parser

**Étapes :**
1. Ouvrir le PDF dans Adobe Acrobat
2. Fichier → Enregistrer sous → Texte (*.txt)
3. Sauvegarder avec le même nom : `TECHNI SO-929237 - ALDI BLAMONT.txt`
4. Placer dans le même dossier que le PDF

Le code détecte automatiquement le `.txt` et l'utilise en fallback.

---

## 🔍 Ordre d'essai automatique

Le code `PopplerPdfExtractor::extractTextFromPdf()` essaie dans cet ordre :

1. **pdftotext -layout** (cherche dans 5 emplacements)
2. **API Poppler C++** (avec layout amélioré)
3. **Fichier .txt existant** (même nom que le PDF)

Si aucune méthode ne fonctionne → message d'erreur.

---

## 🧪 Test après modification

1. **Compiler** le projet (Ctrl+Shift+B)
2. **Lancer** TCHub
3. **Charger** un PDF Lindab
4. **Parser** → observer les logs :

```
[PopplerPdfExtractor] Extraction reussie avec pdftotext (.\pdftotext.exe)
    OU
[PopplerPdfExtractor] Extraction reussie avec API Poppler C++
    OU
[PopplerPdfExtractor] Extraction depuis fichier .txt existant

=== RESUME PARSING LINDAB ===
Lignes totales texte : 303
Lignes avec PCE : 33
Produits extraits : 32
==============================
```

---

## 💡 Recommandation finale

**Pour une solution définitive :**
- Téléchargez `pdftotext.exe` (Solution 1)
- Placez-le dans `x64\Debug\pdftotext.exe` (même dossier que TCHub.exe)
- Copiez toutes les DLLs Poppler à côté

**Pour tester rapidement :**
- Utilisez l'API Poppler améliorée (Solution 2) - déjà intégrée
- Si ça ne marche pas, exportez manuellement en .txt (Solution 3)

Le parser multi-lignes est maintenant **fonctionnel** et n'attend qu'une extraction de texte correcte ! ✨
