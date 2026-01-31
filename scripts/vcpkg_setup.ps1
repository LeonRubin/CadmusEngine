param(
  [switch]$NoActivate
)

$repoRoot = Split-Path -Parent $PSScriptRoot
$defaultVcpkgRoot = Join-Path (Split-Path -Parent $repoRoot) "vcpkg"
$vcpkgRoot = if ($env:VCPKG_ROOT -and (Test-Path $env:VCPKG_ROOT)) { $env:VCPKG_ROOT } elseif (Test-Path $defaultVcpkgRoot) { $defaultVcpkgRoot } else { $null }

if (-not $vcpkgRoot) {
  Write-Error "VCPKG_ROOT is not set and no vcpkg root was found at '$defaultVcpkgRoot'. Set VCPKG_ROOT and try again."
  exit 1
}

$env:VCPKG_ROOT = $vcpkgRoot
$env:VCPKG_DOWNLOADS = Join-Path $repoRoot ".vcpkg\downloads"
$env:VCPKG_DEFAULT_BINARY_CACHE = Join-Path $repoRoot ".vcpkg\bincache"
$artifactRoot = Join-Path $repoRoot ".vcpkg\artifacts"
$toolsRoot = Join-Path $repoRoot ".vcpkg\tools"

New-Item -ItemType Directory -Force -Path $env:VCPKG_DOWNLOADS | Out-Null
New-Item -ItemType Directory -Force -Path $env:VCPKG_DEFAULT_BINARY_CACHE | Out-Null
New-Item -ItemType Directory -Force -Path $artifactRoot | Out-Null
New-Item -ItemType Directory -Force -Path $toolsRoot | Out-Null

$vcpkgExe = Join-Path $vcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
  Write-Error "vcpkg.exe not found at '$vcpkgExe'. Check VCPKG_ROOT."
  exit 1
}

& $vcpkgExe acquire_project --downloads-root $env:VCPKG_DOWNLOADS --x-install-root $artifactRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$activateJson = Join-Path $repoRoot ".vcpkg\activate.json"
& $vcpkgExe activate --downloads-root $env:VCPKG_DOWNLOADS --x-install-root $artifactRoot --json $activateJson | Out-Null
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$activation = Get-Content -Path $activateJson -Raw | ConvertFrom-Json
if ($activation.tools.ninja) {
  $ninjaSrc = $activation.tools.ninja
  $ninjaDstDir = Join-Path $toolsRoot "ninja"
  $ninjaDst = Join-Path $ninjaDstDir "ninja.exe"
  New-Item -ItemType Directory -Force -Path $ninjaDstDir | Out-Null
  Copy-Item -Force -Path $ninjaSrc -Destination $ninjaDst
} else {
  Write-Warning "Ninja tool path not found in vcpkg activation output."
}

if (-not $NoActivate) {
  if ($activation.variables.PATH) {
    $env:PATH = $activation.variables.PATH
  }
}

Write-Host "vcpkg artifacts ready. Downloads: $env:VCPKG_DOWNLOADS"
Write-Host "Artifacts: $artifactRoot"
Write-Host "Binary cache: $env:VCPKG_DEFAULT_BINARY_CACHE"
Write-Host "Ninja (stable): $toolsRoot\\ninja\\ninja.exe"
