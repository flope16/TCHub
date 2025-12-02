# Script d'installation de Poppler via vcpkg
# À exécuter une seule fois

Write-Host "Installation de Poppler via vcpkg..." -ForegroundColor Cyan

# Aller dans le dossier vcpkg
Set-Location C:\Dev\vcpkg

# Installer poppler pour x64-windows
.\vcpkg install poppler:x64-windows

Write-Host ""
Write-Host "Installation terminée !" -ForegroundColor Green
Write-Host ""
Write-Host "Poppler sera disponible dans:" -ForegroundColor Yellow
Write-Host "  C:\Dev\vcpkg\installed\x64-windows\include\poppler" -ForegroundColor White
Write-Host "  C:\Dev\vcpkg\installed\x64-windows\lib\" -ForegroundColor White
Write-Host ""
Write-Host "Appuyez sur une touche pour continuer..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
