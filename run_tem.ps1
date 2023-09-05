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

# Directories
$tem_biogeography = "."
$tem_core = "$tem_biogeography/tem_core"
$runs = "$tem_biogeography/runs"

function clean_tem_core {
    Set-Location $tem_core
    Write-Host "*** Removing all .object files ***" -ForegroundColor $CYAN
    Remove-Item *.o
    progress_bar 3
}

function compile_tem {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Compiling TEM ***" -ForegroundColor $CYAN
    & make -f Makefile_biogeo.xtem xtem45_biogeo
    progress_bar 5
}

function copy_executable {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Copying TEM executable to run directory ***" -ForegroundColor $CYAN
    Copy-Item xtem45_biogeo "../runs"
    Set-Location "../"
    progress_bar 6
}

function remove_temout_files {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Removing all .temout files from runs dir ***" -ForegroundColor $CYAN
    Remove-Item "$runs/*.TEMOUT"
    progress_bar 7
}

function prep_run_dir {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Preparing run directory ***" -ForegroundColor $CYAN
    Remove-Item FIRE.csv, FIRE_VARS.csv, MMDI.csv, *.log
    Copy-Item ../biogeography_module/* .
    Copy-Item ../xtran/* .
    progress_bar 8
}

function run_tem_executable {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Running TEM ***" -ForegroundColor $CYAN
    Set-Location $runs
    Set-ExecutionPolicy Bypass -Scope Process -Force
    Start-Process ./xtem45_biogeo -Wait
    progress_bar 60
}

function biogeography_and_post_processing {
    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Running biogeography model ***" -ForegroundColor $CYAN
    & python main.py
    progress_bar 80

    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Running xtran ***" -ForegroundColor $CYAN
    & python xtran.py xbatch.xml
    progress_bar 90

    Write-Host "" -ForegroundColor $NC
    Write-Host "*** Creating Vegetation Maps ***" -ForegroundColor $CYAN
    & python vegetation_maps.py
    progress_bar 95
}

# Function calls
clean_tem_core
compile_tem
copy_executable
remove_temout_files
prep_run_dir
run_tem_executable
biogeography_and_post_processing

# Completion message
Write-Host "***" -ForegroundColor $YELLOW
progress_bar 100
Write-Host "*** Complete! ***" -ForegroundColor $GREEN
Write-Host "*************************************************" -ForegroundColor $YELLOW
