# Set error handling
$ErrorActionPreference = "Stop"

# Define colors
$CYAN = "Cyan"
$GREEN = "Green"
$YELLOW = "Yellow"
$NC = "White" # Reset Color



# Function to display progress bar
function progress_bar {
    param([int]$percentage)

    $width = 50
    $completed = [Math]::Round(($percentage * $width) / 100)
    $remaining = $width - $completed
    Write-Host "Progress : [$('#' * $completed + '-' * $remaining)] $percentage%`r" -ForegroundColor $YELLOW
}

# Header
Write-Host "" -ForegroundColor $NC
Write-Host "*************************************************" -ForegroundColor $GREEN
Write-Host "***       Starting Biogeography Model        ***" -ForegroundColor $GREEN
Write-Host "*************************************************" -ForegroundColor $GREEN

# Set directories
$tem_biogeography = Get-Location
$tem_core = Join-Path $tem_biogeography "tem_core"
$runs = Join-Path $tem_biogeography "runs"

# Function to clean tem_core
function Clean-TemCore {
    Set-Location $tem_core
    Write-Host "*** Removing all .object files ***" -ForegroundColor $CYAN
    Remove-Item -Path *.o -Force -ErrorAction SilentlyContinue
    Remove-Item -Path *.exe -Force -ErrorAction SilentlyContinue
    progress_bar 3
}

Clean-TemCore

# Function to compile tem
function Compile-Tem {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Compiling TEM ***" -ForegroundColor $CYAN
    Start-Process -Wait -FilePath "make" -ArgumentList "-f Makefile_biogeo.xtem xtem45_biogeo"
    progress_bar 5
    
}

Compile-Tem

# Function to copy executable
function Copy-Executable {
   Write-Host "" -ForegroundColor $NC
    Write-Host "*** Copying TEM executable to run directory ***" -ForegroundColor $CYAN
    Copy-Item "xtem45_biogeo.exe" -Destination $runs -Force
    Set-Location $runs
     progress_bar 6
}

Copy-Executable

# Function to remove .TEMOUT files
function Remove-TemoutFiles {
   Write-Host "" -ForegroundColor $NC
    Write-Host "*** Removing all .temout files from runs dir ***" -ForegroundColor $CYAN
    Remove-Item "$runs\*.TEMOUT" -Force -ErrorAction SilentlyContinue
    progress_bar 7
}

Remove-TemoutFiles

# Function to remove unnecessary files
function Remove-UnnecessaryFiles {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Preparing run directory ***" -ForegroundColor $CYAN
    Remove-Item "$runs\FIRE.csv", "$runs\FIRE_VARS.csv", "$runs\MMDI.csv", "$runs\*.log" -Force -ErrorAction SilentlyContinue
    Copy-Item ../biogeography_module/* .
    Copy-Item ../xtran/* .
    progress_bar 8
    
}

Remove-UnnecessaryFiles

# Function to run TEM executable
function Run-TemExecutable {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Running TEM ***" -ForegroundColor $CYAN
    Start-Process -Wait -FilePath ".\xtem45_biogeo.exe"
    progress_bar 60
}

Run-TemExecutable



# Function for biogeography and post-processing
function Biogeography-And-PostProcessing {
    $pwd = Get-Location

    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Running biogeography model ***" -ForegroundColor $CYAN
    & python.exe main.py -Wait
    
    
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Running xtran ***" -ForegroundColor $CYAN
    & python.exe xtran.py xbatch.xml -Wait

    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Creating Vegetation Maps ***" -ForegroundColor $CYAN
    & python.exe vegetation_maps.py -Wait
    
    progress_bar 90 
}

Biogeography-And-PostProcessing

Write-Host "***" -ForegroundColor $YELLOW
progress_bar 100
Write-Host "*** Complete! ***" -ForegroundColor $GREEN
Write-Host "*************************************************" -ForegroundColor $YELLOW
