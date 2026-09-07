[CmdletBinding()]
param(
    [string]$XilinxRoot = 'D:\install\xilinx',
    [string]$XilinxVersion = '2021.1'
)

$ErrorActionPreference = 'Stop'
$vitisRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$bootDir = Join-Path $vitisRoot 'boot'
$bootgen = Join-Path $XilinxRoot "Vivado\$XilinxVersion\bin\bootgen.bat"
$bif = Join-Path $bootDir 'qiming_yolo.bif'
$output = Join-Path $bootDir 'BOOT.BIN'

foreach ($required in @($bootgen, $bif, (Join-Path $vitisRoot 'output\yolo_qiming.elf'), (Join-Path $vitisRoot 'workspace_mindc\mindc_yolo_platform\export\mindc_yolo_platform\sw\mindc_yolo_platform\boot\fsbl.elf'))) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required boot input not found: $required"
    }
}

Push-Location $bootDir
try {
    & $bootgen -arch zynq -image (Split-Path -Leaf $bif) -o (Split-Path -Leaf $output) -w on
    if ($LASTEXITCODE -ne 0) {
        throw 'Bootgen failed'
    }
} finally {
    Pop-Location
}

Write-Output "BOOT_IMAGE_COMPLETE=$output"
