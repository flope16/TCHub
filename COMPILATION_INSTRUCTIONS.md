# Instructions de compilation pour minizip

Le projet a été configuré pour utiliser minizip afin de créer de vrais fichiers XLSX.

## Prérequis

Minizip doit être installé via vcpkg. Si ce n'est pas déjà fait, exécutez :

```cmd
cd C:\Dev\vcpkg
vcpkg install zlib:x64-windows
```

Note : zlib inclut minizip dans vcpkg.

## Compilation

1. Ouvrez Visual Studio 2022
2. Ouvrez le fichier `TCHub.sln`
3. Sélectionnez la configuration **Debug x64** ou **Release x64**
4. Cliquez sur **Build > Rebuild Solution** (F7)

⚠️ **IMPORTANT** : Vous devez faire une **recompilation complète** (Rebuild) car la définition `USE_MINIZIP` a été ajoutée au projet.

## Vérification

Après la compilation, lorsque vous convertissez un PDF Lindab, vous devriez voir dans les logs :

```
[XlsxWriter] Tentative de création ZIP avec minizip...
[XlsxWriter] Fichier XLSX créé avec succès via minizip
```

Au lieu de :

```
[XlsxWriter] Tentative avec PowerShell...
```

## Résultat attendu

Le fichier généré devrait avoir l'extension `.xlsx` et être directement ouvrable dans Excel sans erreur.

## En cas de problème

Si la compilation échoue avec des erreurs liées à minizip :
1. Vérifiez que vcpkg est bien configuré
2. Vérifiez que zlib est installé : `vcpkg list | findstr zlib`
3. Vérifiez le chemin : `C:\Dev\vcpkg\installed\x64-windows\include\minizip\zip.h` doit exister
