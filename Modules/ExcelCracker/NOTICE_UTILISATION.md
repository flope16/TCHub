# Notice d'Utilisation - Excel Cracker

## Introduction

Excel Cracker est un module intégré dans TCHub qui permet de :
1. **Supprimer la protection des feuilles** d'un fichier Excel non crypté
2. **Forcer le mot de passe** d'un fichier Excel crypté par brute-force

Ce module a été développé en C++ comme alternative aux scripts Python originaux.

---

## 📋 Méthode 1 : Suppression de Protection des Feuilles

### Quand l'utiliser ?
- Vous avez un fichier Excel (.xlsx) avec des feuilles protégées
- Le fichier **n'est PAS crypté** (vous pouvez l'ouvrir sans mot de passe)
- Vous voulez déverrouiller les cellules pour pouvoir les modifier

### Étapes à suivre :

#### Via l'interface graphique :

1. **Lancer TCHub**
   - Double-cliquer sur `TCHub.exe`

2. **Ouvrir Excel Cracker**
   - Dans la fenêtre principale, cliquer sur "Ouvrir" dans la section "Excel Cracker"

3. **Configurer le mode**
   - Sélectionner "Supprimer protection des feuilles" dans le menu déroulant "Mode"

4. **Sélectionner le fichier**
   - Cliquer sur "Parcourir..."
   - Choisir votre fichier Excel (.xlsx)

5. **Fermer tous les fichiers Excel ouverts**
   - ⚠️ **IMPORTANT** : Fermer tous les fichiers Excel avant de continuer

6. **Traiter le fichier**
   - Cliquer sur "Traiter"
   - Attendre quelques secondes

7. **Récupérer le fichier déverrouillé**
   - Un nouveau fichier `VotreFichier_unprotected.xlsx` est créé
   - Le fichier original reste intact
   - Une sauvegarde `VotreFichier.xlsx.backup` est également créée

### Exemple concret :

```
Fichier d'entrée:  Devis_Client_2024.xlsx (feuilles protégées)

Après traitement:
✅ Devis_Client_2024.xlsx (original, intact)
✅ Devis_Client_2024.xlsx.backup (sauvegarde)
✅ Devis_Client_2024_unprotected.xlsx (NOUVEAU, déverrouillé)
```

### Que se passe-t-il sous le capot ?

1. Le fichier .xlsx est décompressé (c'est un fichier ZIP)
2. Les fichiers XML des feuilles sont analysés
3. Les balises `<sheetProtection>` sont supprimées
4. Le tout est recompressé en un nouveau fichier .xlsx

### En cas d'erreur :

**Erreur : "Le fichier ne semble pas être crypté"**
- Le fichier est probablement crypté → Utilisez la Méthode 2

**Erreur : "Aucune protection trouvée"**
- Les feuilles ne sont pas protégées
- Pas besoin d'utiliser cet outil

**Erreur : "minizip n'est pas activé"**
- Contactez l'administrateur système
- Le logiciel doit être recompilé avec minizip

---

## 🔐 Méthode 2 : Brute-Force de Mot de Passe

### Quand l'utiliser ?
- Vous avez un fichier Excel **crypté**
- Vous ne pouvez **PAS** ouvrir le fichier sans mot de passe
- Le mot de passe est **court** (1-4 caractères recommandé)

### ⚠️ AVERTISSEMENTS IMPORTANTS

1. **Le brute-force peut prendre TRÈS LONGTEMPS**
   - Mot de passe de 4 caractères (a-z) : ~475,000 tentatives
   - Mot de passe de 5 caractères (a-z) : ~12 millions de tentatives
   - Mot de passe de 6+ caractères : **PLUSIEURS JOURS**

2. **Microsoft Excel doit être installé**
   - Cette méthode utilise COM Automation
   - Nécessite Excel installé sur l'ordinateur

3. **Usage légal uniquement**
   - Utilisez uniquement sur vos propres fichiers
   - Obtenir l'autorisation si ce n'est pas votre fichier

### Étapes à suivre :

#### Via l'interface graphique :

1. **Lancer TCHub et ouvrir Excel Cracker**
   - (voir Méthode 1 étapes 1-2)

2. **Configurer le mode**
   - Sélectionner "Brute-force mot de passe" dans le menu déroulant "Mode"
   - Une section "Configuration Brute-Force" apparaît

3. **Configurer les paramètres**

   **Longueur du mot de passe :**
   - `Min`: Longueur minimale probable (ex: 1)
   - `Max`: Longueur maximale probable (ex: 4)

   ⚠️ **Recommandation : Ne PAS dépasser 4-5 caractères**

   **Jeu de caractères :**
   - Cocher les types de caractères possibles :
     - `a-z` : Lettres minuscules
     - `A-Z` : Lettres majuscules
     - `0-9` : Chiffres
     - `!@#$%` : Caractères spéciaux

   💡 **Astuce** : Moins de types cochés = Plus rapide

   **Exemples de configuration :**

   ```
   Configuration RAPIDE (test):
   Min: 1, Max: 3
   Caractères: a-z uniquement
   Temps estimé: ~1 minute
   Tentatives: ~18,000
   ```

   ```
   Configuration MOYENNE:
   Min: 1, Max: 4
   Caractères: a-z uniquement
   Temps estimé: ~5-10 minutes
   Tentatives: ~475,000
   ```

   ```
   Configuration LENTE (déconseillé):
   Min: 1, Max: 5
   Caractères: a-z + A-Z
   Temps estimé: PLUSIEURS HEURES
   Tentatives: ~380 millions
   ```

4. **Sélectionner le fichier crypté**
   - Cliquer sur "Parcourir..."
   - Choisir votre fichier Excel crypté

5. **Fermer tous les fichiers Excel**
   - ⚠️ **CRITIQUE** : Fermer toutes les instances d'Excel

6. **Lancer le brute-force**
   - Cliquer sur "Traiter"
   - Le processus démarre
   - Vous verrez :
     - Nombre de tentatives
     - Mot de passe actuellement testé

7. **Surveiller la progression**
   - La fenêtre affiche les tentatives en temps réel
   - Vous pouvez cliquer sur "Arrêter" pour annuler

8. **Résultat**
   - **Si trouvé** : Le mot de passe s'affiche dans une boîte de dialogue
   - **Si non trouvé** : Message d'échec après avoir testé toutes les combinaisons

### Estimation du temps :

| Longueur | Caractères | Combinaisons | Temps estimé* |
|----------|-----------|--------------|---------------|
| 1-3      | a-z       | ~18,000      | ~1-2 min      |
| 1-4      | a-z       | ~475,000     | ~5-10 min     |
| 1-5      | a-z       | ~12M         | ~1-2 heures   |
| 1-4      | a-z+A-Z   | ~7.9M        | ~30-60 min    |
| 1-4      | a-z+0-9   | ~1.7M        | ~10-20 min    |
| 1-5      | a-z+A-Z+0-9 | ~916M      | **JOURS**     |

\* Temps estimé sur un ordinateur standard (peut varier)

### Exemple concret :

**Scénario :** Vous savez que le mot de passe est un mot simple en minuscules de 4 lettres maximum.

```
Configuration :
- Mode: Brute-force mot de passe
- Fichier: Budget_2024_crypte.xlsx
- Min: 1
- Max: 4
- Caractères: a-z uniquement

Lancement...
[00:00:30] Tentatives: 10,000 | Mot de passe: abcd
[00:01:15] Tentatives: 25,000 | Mot de passe: bcde
[00:03:42] Tentatives: 89,543 | Mot de passe: test

🎉 Mot de passe trouvé: "test" après 89,543 tentatives!
```

### En cas d'erreur :

**Erreur : "Le fichier ne semble pas être crypté"**
- Le fichier n'est pas crypté → Utilisez la Méthode 1

**Erreur : "Microsoft Excel n'est pas installé"**
- Installer Microsoft Excel sur l'ordinateur

**Le processus est très lent**
- Réduire la longueur maximale
- Réduire le nombre de types de caractères
- Être patient !

---

## 🛡️ Sécurité et Légalité

### ⚠️ ATTENTION

- **Usage légal uniquement**
  - Ne pas utiliser sur des fichiers qui ne vous appartiennent pas
  - Obtenir l'autorisation écrite si nécessaire

- **Antivirus**
  - Certains antivirus peuvent bloquer l'outil (faux positif)
  - Le brute-force peut être détecté comme suspect

- **Mot de passe fort**
  - Si votre mot de passe a >6 caractères avec majuscules/chiffres/symboles
  - Le brute-force sera **IMPOSSIBLE** dans un temps raisonnable
  - C'est une **bonne chose** pour la sécurité !

---

## 📞 Support

### Problèmes courants :

1. **"MINIZIP n'est pas activé"**
   - Voir `INSTALL_MINIZIP.md` pour l'installation
   - Recompiler le projet avec `USE_MINIZIP`

2. **Le fichier ne s'ouvre pas**
   - Vérifier que le fichier est bien un .xlsx
   - Essayer de l'ouvrir manuellement dans Excel
   - Vérifier qu'il n'est pas corrompu

3. **Le brute-force ne trouve rien**
   - Le mot de passe est probablement plus complexe que prévu
   - Augmenter la longueur maximale (mais temps ++)
   - Ajouter d'autres types de caractères

4. **Excel se lance et se ferme sans cesse**
   - C'est normal pendant le brute-force
   - Chaque tentative ouvre/ferme Excel
   - Ne pas interrompre le processus

---

## 📝 Fichiers créés

### Méthode 1 (Suppression de protection) :
```
VotreFichier.xlsx                 → Original (intact)
VotreFichier.xlsx.backup          → Sauvegarde automatique
VotreFichier_unprotected.xlsx     → Nouveau fichier déverrouillé
```

### Méthode 2 (Brute-force) :
```
Aucun fichier créé
Seul le mot de passe est affiché
```

---

## 🔧 Prérequis Techniques

### Pour la suppression de protection :
- ✅ Windows 10 ou supérieur
- ✅ TCHub compilé avec minizip

### Pour le brute-force :
- ✅ Windows 10 ou supérieur
- ✅ TCHub compilé avec minizip
- ✅ **Microsoft Excel installé** (2013, 2016, 2019, 365)

---

## 🎯 Cas d'usage recommandés

### ✅ Utiliser cet outil pour :
- Déverrouiller vos propres fichiers dont vous avez oublié le mot de passe
- Supprimer la protection de feuilles sur vos fichiers de travail
- Tester la sécurité de vos mots de passe Excel

### ❌ Ne PAS utiliser pour :
- Fichiers appartenant à d'autres personnes sans autorisation
- Fichiers professionnels sans accord de votre entreprise
- Contourner des protections légitimes

---

**Bon courage et utilisez cet outil de manière responsable ! 🔐**
