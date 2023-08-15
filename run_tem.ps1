# Set error handling
$ErrorActionPreference = "Stop"

Write-Host "<<< Starting Biogeography Model >>>"

# Set directories
$tem_biogeography = Get-Location
$tem_core = Join-Path $tem_biogeography "tem_core"
$runs = Join-Path $tem_biogeography "runs"

# Function to clean tem_core
function Clean-TemCore {
    Set-Location $tem_core
    Write-Host "Removing all .o files..."
    Remove-Item -Path *.o -Force -ErrorAction SilentlyContinue
    Write-Host "Done!"
}

Clean-TemCore

# Function to compile tem
function Compile-Tem {
    Write-Host "Compiling tem..."
    Start-Process -Wait -FilePath "make" -ArgumentList "-f Makefile_biogeo.xtem xtem45_biogeo"
    Write-Host "Done!"
}

Compile-Tem

# Function to copy executable
function Copy-Executable {
    Write-Host "Copying tem executable to run directory..."
    Copy-Item "xtem45_biogeo.exe" -Destination $runs -Force
    Set-Location $runs
    Write-Host "Done!"
}

Copy-Executable

# Function to remove .TEMOUT files
function Remove-TemoutFiles {
    Write-Host "Removing all .TEMOUT files from directory..."
    Remove-Item "$runs\*.TEMOUT" -Force -ErrorAction SilentlyContinue
    Write-Host "Done!"
}

Remove-TemoutFiles

# Function to remove unnecessary files
function Remove-UnnecessaryFiles {
    Write-Host "Removing unnecessary files..."
    Remove-Item "$runs\FIRE.csv", "$runs\FIRE_VARS.csv", "$runs\MMDI.csv", "$runs\*.log" -Force -ErrorAction SilentlyContinue
    Write-Host "Done!"
}

Remove-UnnecessaryFiles

# Function to run TEM executable
function Run-TemExecutable {
    Write-Host "Running TEM executable..."
    Start-Process -Wait -FilePath ".\xtem45_biogeo.exe"
    Write-Host "Done!"
}

Run-TemExecutable

Write-Host "TEM run complete"

# Function for biogeography and post-processing
function Biogeography-And-PostProcessing {
    $pwd = Get-Location

    Write-Host "Running biogeography model..."
    python.exe main.py
    Write-Host "Done!"
    Write-Host "Creating Vegetation Maps..."
    python.exe vegetation_maps.py
    Write-Host "Done!"
    Write-Host "Running xtran..."
    $xbatchContent = Get-Content "xbatch.xml" -Raw
    $xbatchContent | python.exe xtran.py
    Write-Host "Done!"
}

Biogeography-And-PostProcessing

Write-Host "Complete!"