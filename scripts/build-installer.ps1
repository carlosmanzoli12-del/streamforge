[CmdletBinding()]
param(
    [string]$ObsSdkRoot = $env:OBS_SDK_ROOT,
    [string]$BuildDirectory = "build",
    [ValidateSet("Release", "RelWithDebInfo", "Debug")]
    [string]$Configuration = "Release",
    [string]$Generator = "Visual Studio 17 2022"
)

$ErrorActionPreference = "Stop"

function Require-Command([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Ferramenta obrigatória não encontrada no PATH: $Name"
    }
}

Require-Command "cmake"
Require-Command "cpack"
Require-Command "makensis"

if ([string]::IsNullOrWhiteSpace($ObsSdkRoot)) {
    throw "Informe -ObsSdkRoot ou defina OBS_SDK_ROOT com o SDK/dependências de desenvolvimento do OBS 30.0.2."
}
if (-not (Test-Path $ObsSdkRoot -PathType Container)) {
    throw "Pasta do SDK não encontrada: $ObsSdkRoot"
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = Join-Path $repoRoot $BuildDirectory
$sdkPath = (Resolve-Path $ObsSdkRoot).Path

Write-Host "Configurando StreamForge Vertical..."
Write-Host "SDK: $sdkPath"
Write-Host "Build: $buildPath"

cmake -S $repoRoot -B $buildPath -G $Generator -A x64 "-DCMAKE_PREFIX_PATH=$sdkPath" -DSTREAMFORGE_BUILD_OBS_PLUGIN=ON -DSTREAMFORGE_BUILD_INSTALLER=ON
if ($LASTEXITCODE -ne 0) { throw "Falha na configuração CMake." }

cmake --build $buildPath --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar o plugin." }

Push-Location $buildPath
try {
    cpack --config CPackConfig.cmake -C $Configuration -G NSIS
    if ($LASTEXITCODE -ne 0) { throw "Falha ao gerar o instalador NSIS." }
}
finally {
    Pop-Location
}

$installers = Get-ChildItem -Path $buildPath -Filter "*.exe" -File | Where-Object { $_.Name -match "StreamForge" }
if (-not $installers) { throw "Nenhum instalador StreamForge*.exe foi encontrado em $buildPath." }

Write-Host ""
Write-Host "Instalador gerado:"
$installers | ForEach-Object { Write-Host $_.FullName }
