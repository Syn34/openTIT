# Build the tools, recompile your EBOOT and build the game (Windows / PowerShell).
#
#   .\tools\build.ps1 -Dump "D:\Dumps\Sly 4"
#
# Requires: Visual Studio 2022 (Desktop C++), CMake, Ninja on PATH.
# Run from a "Developer PowerShell for VS 2022" so cl.exe is available.
param(
    [Parameter(Mandatory = $true)][string]$Dump,
    [string]$Gen = "gen",
    [string]$Config = "Release"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    $eboot = Join-Path $Dump "PS3_GAME\USRDIR\EBOOT.elf"
    if (-not (Test-Path $eboot)) {
        throw "Decrypted executable not found: $eboot`nDecrypt EBOOT.BIN with RPCS3 (Utilities > Decrypt PS3 Binaries) and save it as EBOOT.elf."
    }

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=$Config
    cmake --build build
    .\build\opentit_tests.exe

    .\build\opentit-recomp.exe recompile $eboot -o $Gen

    # Optional LLE firmware modules (decrypted with RPCS3), e.g. firmware\libsre.prx
    $base = 0x01800000
    $prxs = @(Get-ChildItem (Join-Path $Dump "firmware") -Filter *.prx -ErrorAction SilentlyContinue) +
            @(Get-ChildItem $Dump -Filter *.prx -ErrorAction SilentlyContinue)
    if ($prxs.Count -gt 0) {
        foreach ($prx in $prxs | Sort-Object Name -Unique) {
            $name = $prx.BaseName
            .\build\opentit-recomp.exe recompile $prx.FullName -o (Join-Path $Gen "modules\$name") --base ('{0:X}' -f $base) --name $name
            $base += 0x00400000
        }
    }

    $genAbs = (Resolve-Path $Gen).Path
    cmake -S . -B build-game -G Ninja -DCMAKE_BUILD_TYPE=$Config -DOPENTIT_BUILD_TESTS=OFF "-DOPENTIT_GENERATED_DIR=$genAbs"
    cmake --build build-game

    Write-Host "`nDone. Run:  .\build-game\opentit.exe `"$Dump`" --log info"
}
finally {
    Pop-Location
}
