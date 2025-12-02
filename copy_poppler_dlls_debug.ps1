# Script PowerShell pour copier les DLLs Poppler vers Debug

$vcpkgPath = "C:\Dev\vcpkg\installed\x64-windows\bin"
$debugPath = "C:\Dev\TCHub\x64\Debug"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Copie des DLL Poppler vers Debug" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Vérifier que le dossier vcpkg existe
if (-not (Test-Path $vcpkgPath)) {
    Write-Host "[ERREUR] Le dossier vcpkg n'existe pas: $vcpkgPath" -ForegroundColor Red
    exit 1
}

# Créer le dossier Debug s'il n'existe pas
if (-not (Test-Path $debugPath)) {
    Write-Host "[CREATION] Dossier Debug..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $debugPath -Force | Out-Null
}

# Liste des DLLs Poppler et dépendances
$dlls = @(
    "poppler-cpp.dll",
    "poppler.dll",
    "freetype.dll",
    "zlib1.dll",
    "jpeg62.dll",
    "libpng16.dll",
    "openjp2.dll",
    "tiff.dll",
    "lzma.dll",
    "bz2.dll",
    "lcms2.dll",
    "libiconv.dll",
    "libintl.dll",
    "charset.dll"
)

$copiedCount = 0
$notFoundCount = 0

foreach ($dll in $dlls) {
    $sourcePath = Join-Path $vcpkgPath $dll
    $destPath = Join-Path $debugPath $dll

    if (Test-Path $sourcePath) {
        Copy-Item -Path $sourcePath -Destination $destPath -Force
        Write-Host "[OK] $dll" -ForegroundColor Green
        $copiedCount++
    } else {
        Write-Host "[SKIP] $dll (non trouve)" -ForegroundColor Yellow
        $notFoundCount++
    }
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host " Resultat" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "DLLs copiees: $copiedCount" -ForegroundColor Green
Write-Host "DLLs non trouvees: $notFoundCount" -ForegroundColor Yellow
Write-Host ""
Write-Host "Les DLL Poppler ont ete copiees dans:" -ForegroundColor Cyan
Write-Host $debugPath -ForegroundColor White
