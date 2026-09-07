[CmdletBinding()]
param(
    [string]$XilinxRoot = 'C:\Xilinx',
    [string]$XilinxVersion = '2021.2'
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$vivado = Join-Path $XilinxRoot "Vivado\$XilinxVersion\bin\vivado.bat"
$xsct = Join-Path $XilinxRoot "Vitis\$XilinxVersion\bin\xsct.bat"

foreach ($required in @($vivado, $xsct)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required tool not found: $required"
    }
}

Write-Output "STAGE=vivado_hardware START"
& $vivado -mode batch -source (Join-Path $root 'vivado_prj\scripts\build_hardware.tcl')
if ($LASTEXITCODE -ne 0) { throw "vivado hardware build failed with exit code $LASTEXITCODE" }
Write-Output "STAGE=vivado_hardware DONE"

Write-Output "STAGE=vitis_platform START"
& $xsct (Join-Path $root 'vitis_prj\scripts\build_platform.tcl')
if ($LASTEXITCODE -ne 0) { throw "vitis platform build failed with exit code $LASTEXITCODE" }
Write-Output "STAGE=vitis_platform DONE"

Write-Output "STAGE=app START"
& (Join-Path $root 'vitis_prj\scripts\build_app.ps1') -XilinxRoot $XilinxRoot -XilinxVersion $XilinxVersion
if ($LASTEXITCODE -ne 0) { throw "app build failed with exit code $LASTEXITCODE" }
Write-Output "STAGE=app DONE"

Write-Output "STAGE=boot START"
& (Join-Path $root 'vitis_prj\scripts\build_boot.ps1') -XilinxRoot $XilinxRoot -XilinxVersion $XilinxVersion
if ($LASTEXITCODE -ne 0) { throw "boot image build failed with exit code $LASTEXITCODE" }
Write-Output "STAGE=boot DONE"

Write-Output "FULL_BUILD_COMPLETE"
