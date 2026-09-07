[CmdletBinding()]
param(
    [string]$XilinxRoot = 'D:\install\xilinx',
    [string]$XilinxVersion = '2021.1',
    [switch]$Diagnostic
)

$ErrorActionPreference = 'Stop'
$vitisRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourceDir = Join-Path $vitisRoot 'app\src'
$workspace = Join-Path $vitisRoot 'workspace_mindc'
$outputDir = Join-Path $vitisRoot 'output'
$bspRoot = Join-Path $workspace 'mindc_yolo_platform\ps7_cortexa9_0\standalone_domain\bsp\ps7_cortexa9_0'
$spec = Join-Path $workspace 'mindc_yolo_platform\zynq_fsbl\Xilinx.spec'
$toolDir = Join-Path $XilinxRoot "Vitis\$XilinxVersion\gnu\aarch32\nt\gcc-arm-none-eabi\bin"
$compiler = Join-Path $toolDir 'arm-none-eabi-g++.exe'
$sizeTool = Join-Path $toolDir 'arm-none-eabi-size.exe'

foreach ($required in @($compiler, $spec, (Join-Path $bspRoot 'include\xparameters.h'), (Join-Path $bspRoot 'lib\libxil.a'))) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required build input not found: $required"
    }
}

New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$suffix = if ($Diagnostic) { '_diag' } else { '' }
$defines = if ($Diagnostic) { @('-DQIMING_DIAGNOSTIC') } else { @() }
$common = @(
    '-mcpu=cortex-a9', '-mfpu=vfpv3', '-mfloat-abi=hard', '-O3', '-std=gnu++11',
    '-ffunction-sections', '-fdata-sections', '-fno-tree-loop-distribute-patterns',
    ('-I' + $sourceDir), ('-I' + (Join-Path $bspRoot 'include'))
) + $defines
$sources = @('basic_op.cpp', 'boot_diag.cpp', 'debug.cpp', 'image_io.cpp', 'main.cpp', 'sd_io.cpp', 'yolo_postprocess.cpp')
$objects = @()

foreach ($source in $sources) {
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($source)
    $object = Join-Path $outputDir ($stem + $suffix + '.o')
    & $compiler @common -c (Join-Path $sourceDir $source) -o $object
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed: $source"
    }
    $objects += $object
}

$elf = Join-Path $outputDir ('yolo_qiming' + $suffix + '.elf')
$map = Join-Path $outputDir ('yolo_qiming' + $suffix + '.map')
$linkArgs = @(
    '-mcpu=cortex-a9', '-mfpu=vfpv3', '-mfloat-abi=hard', '-nodefaultlibs',
    ('-specs=' + $spec), ('-Wl,-T,' + (Join-Path $sourceDir 'lscript.ld')),
    '-Wl,--gc-sections', ('-Wl,-Map,' + $map), ('-L' + (Join-Path $bspRoot 'lib'))
)
& $compiler @linkArgs @objects '-Wl,--start-group' '-lxilffs' '-lxil' '-lstdc++' '-lm' '-lgcc' '-lc' '-Wl,--end-group' -o $elf
if ($LASTEXITCODE -ne 0) {
    throw 'Application link failed'
}

& $sizeTool $elf
Write-Output "APPLICATION_BUILD_COMPLETE=$elf"
