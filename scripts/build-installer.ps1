param(
    [Parameter(Mandatory = $true)]
    [string]$ObsSdkRoot,
    [string]$BuildDirectory = "build",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw "CMake não foi encontrado no PATH." }
if (-not (Get-Command makensis -ErrorAction SilentlyContinue)) { throw "NSIS (makensis) não foi encontrado. Instale o NSIS e adicione a pasta ao PATH." }
if (-not (Test-Path $ObsSdkRoot)) { throw "Diretório do SDK do OBS não encontrado: $ObsSdkRoot" }

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $root $BuildDirectory

cmake -S $root -B $buildPath -A x64 -DSTREAMFORGE_BUILD_OBS_PLUGIN=ON -DSTREAMFORGE_BUILD_INSTALLER=ON "-DCMAKE_PREFIX_PATH=$ObsSdkRoot"
if ($LASTEXITCODE -ne 0) { throw "Falha na configuração CMake." }

cmake --build $buildPath --config $Configuration
if ($LASTEXITCODE -ne 0) { throw "Falha na compilação do plugin." }

Push-Location $buildPath
try {
    cpack -C $Configuration -G NSIS
    if ($LASTEXITCODE -ne 0) { throw "Falha ao gerar o instalador NSIS." }
} finally { Pop-Location }

Write-Host "Build e empacotamento concluídos. Verifique o instalador gerado em: $buildPath"
