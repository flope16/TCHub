# Guide d'utilisation - PDF Parser Lindab

## 🎯 Pour parser votre PDF immédiatement

### Option 1 : Créer un fichier .txt (recommandé pour tester)

1. Ouvrez votre PDF dans Adobe Reader ou un autre lecteur
2. Sélectionnez tout le texte (Ctrl+A) et copiez (Ctrl+C)
3. Créez un nouveau fichier texte avec **exactement le même nom** que votre PDF
   - Exemple : Si votre PDF s'appelle `TECHNI SO-929237 - ALDI BLAMONT.pdf`
   - Créez `TECHNI SO-929237 - ALDI BLAMONT.txt` dans le **même dossier**
4. Collez le texte copié dans ce fichier .txt
5. Lancez TCHub et parsez le PDF normalement

### Option 2 : Installer pdftotext (pour automatiser)

1. Téléchargez **Poppler for Windows** :
   - https://github.com/oschwartz10612/poppler-windows/releases
   - Prenez `Release-XX.XX.X-0.zip`

2. Extrayez l'archive (par exemple dans `C:\Program Files\poppler`)

3. Ajoutez au PATH Windows :
   - Windows + R → `sysdm.cpl` → Onglet "Avancé"
   - Variables d'environnement
   - Dans "Variables système", double-cliquez sur "Path"
   - Nouveau → `C:\Program Files\poppler\Library\bin`
   - OK partout

4. Redémarrez TCHub - il utilisera automatiquement pdftotext !

## 📋 Format attendu (Lindab)

Le parseur recherche des lignes avec ce format :
```
Ligne  N°Article  Désignation              Qté     Prix    Montant
1      224931     SR 200 3000 GALV         10,00   PCE     18,90    188,99
                   Conduit rigide...
```

**Points clés** :
- Présence du marqueur "PCE"
- Référence article sur 6 chiffres
- Format français (virgule pour décimales)

## 🔍 Test rapide

Créez un fichier `test_lindab.txt` avec ce contenu :

```
Ligne N° article    Désignation                                         Qté         Prix HT     Montant Net HT

1     224931        SR          200    3000   GALV
                    Conduit rigide circulaire galva
                    D=200mm Lg=3000mm                                    10,00  PCE  18,90       188,99

2     224929        SR          160    3000   GALV
                    Conduit rigide circulaire galva
                    D=160mm Lg=3000mm                                     2,00  PCE  14,59        29,17
```

Enregistrez-le et parsez avec TCHub !

## ✅ Résultat

Le fichier Excel généré contiendra :
| Référence | Désignation | Quantité | Prix HT |
|-----------|-------------|----------|---------|
| 224931    | SR          | 10.00    | 18.90   |
| 224929    | SR          | 2.00     | 14.59   |

## ❓ Dépannage

**"Aucune donnée extraite"** :
- Vérifiez que le texte contient "PCE"
- Vérifiez le format des références (6 chiffres)
- Essayez l'option 1 (fichier .txt manuel)

**Labels invisibles dans l'interface** :
- Rebuild le projet (Ctrl+Shift+B)
- Les labels sont maintenant avec fond transparent et padding

**DLL manquantes** :
- Le build copie automatiquement les DLL Qt
- Ou lancez `copy_qt_dlls_debug.bat` manuellement
