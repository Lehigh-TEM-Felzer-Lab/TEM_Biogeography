# Define colors
$CYAN = "`e[0;36m"
$GREEN = "`e[0;32m"
$YELLOW = "`e[0;33m"
$NC = "`e[0m" # No Color

# Function to display progress bar
function progress_bar {
    param([int]$percentage)

    $width = 50
    $completed = [Math]::Round(($percentage * $width) / 100)
    $remaining = $width - $completed
    Write-Host "${YELLOW}Progress : [$('-' * $completed + '#' * $remaining)] $percentage%`r${NC}"
}

# Header
Write-Host ""
Write-Host "${GREEN}*************************************************${NC}"
Write-Host "${GREEN}***       Starting Biogeography Model        ***${NC}"
Write-Host "${GREEN}*************************************************${NC}"

# Directories
$tem_biogeography = "."
$tem_core = "$tem_biogeography/tem_core"
$runs = "$tem_biogeography/runs"

function clean_tem_core {
    Set-Location $tem_core
    Write-Host "${CYAN}*** Removing all .object files ***${NC}"
    Remove-Item *.o
    progress_bar 3
}

function compile_tem {
    Write-Host ""
    Write-Host "${CYAN}*** Compiling TEM ***${NC}"
    & make -f Makefile_biogeo.xtem xtem45_biogeo
    progress_bar 5
}

function copy_executable {
    Write-Host ""
    Write-Host "${CYAN}*** Copying TEM executable to run directory ***${NC}"
    Copy-Item xtem45_biogeo "../runs"
    Set-Location "../"
    progress_bar 6
}

function remove_temout_files {
    Write-Host ""
    Write-Host "${CYAN}*** Removing all .temout files from runs dir ***${NC}"
    Remove-Item "$runs/*.TEMOUT"
    progress_bar 7
}

function prep_run_dir {
    Write-Host ""
    Write-Host "${CYAN}*** Preparing run directory ***${NC}"
    Remove-Item FIRE.csv, FIRE_VARS.csv, MMDI.csv, *.log
    Copy-Item ../biogeography_module/* .
    Copy-Item ../xtran/* .
    progress_bar 8
}

function run_tem_executable {
    Write-Host ""
    Write-Host "${CYAN}*** Running TEM ***${NC}"
    Set-Location $runs
    Set-ExecutionPolicy Bypass -Scope Process -Force
    Start-Process ./xtem45_biogeo -Wait
    progress_bar 60
}

function biogeography_and_post_processing {
    Write-Host ""
    Write-Host "${CYAN}*** Running biogeography model ***${NC}"
    & python main.py
    progress_bar 80

    Write-Host ""
    Write-Host "${CYAN}*** Running xtran ***${NC}"
    & python xtran.py xbatch.xml
    progress_bar 90

    Write-Host ""
    Write-Host "${CYAN}*** Creating Vegetation Maps ***${NC}"
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
Write-Host "${YELLOW}***${NC}"
progress_bar 100
Write-Host "${GREEN}*** Complete! ***${NC}"
Write-Host "${YELLOW}*************************************************${NC}"
