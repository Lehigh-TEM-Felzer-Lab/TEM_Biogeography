# TEM Biogeography

Potential vegetation from the Terrestrial Ecosystem Model (TEM). The workflow runs TEM for land-cover cohorts, compares the annual net primary production (NPP) of the cohorts, and applies the climate limits of plant functional types (PFT) to select the potential vegetation.

The model is the version 4.5 core of [XTEM](https://github.com/Lehigh-TEM-Felzer-Lab/XTEM) (`src/` holds the same files). The case builder and launchers come from [TEM_UW_APP](https://github.com/Lehigh-TEM-Felzer-Lab/TEM_UW_APP). `case_submit.sh` reads `run_config.yaml`, stages the NetCDF inputs, writes `tem_config.yaml`, runs `xtem` and then the PFT selection.

## Repository layout

| Path | Content |
|---|---|
| `src/physics/` | model science and the records it uses: vegetation, soil, microbes, evapotranspiration, fire, land use, the cell and cohort classes, constants, time step |
| `src/io/` | everything that touches files or processes: YAML, NetCDF, restart state, input checks, log, diagnostics transport, MPI layer (`mpi_serial.cpp`, `mpi_parallel.cpp`) |
| `src/driver/` | `xtem_gds.cpp`: the main program, which wires `physics` and `io` together |
| `CONTRIBUTING.md` | source layout, checks, and that `src/physics`, `src/io`, `src/driver` are a copy of XTEM |
| `.github/` | CI (`workflows/ci.yml`), the parity check against XTEM (`workflows/core-parity.yml`), `layering.yaml` and its script |
| `case_submit.sh`, `launchers/` | entry point, local and SLURM launchers |
| `src/*.py` | case builder: regridding, units, `tem_config.yaml`, output conversion (Python, next to the C++ directories above; `src/driver.py` is the case builder, `src/driver/` is the model's main program) |
| `biogeo/` | PFT selection, summaries and maps |
| `config/run_config.yaml` | case keys, defaults and comments |
| `templates/tem_config.yaml` | model keys, with comments |
| `params/` | YAML parameter files. Runs use `gdstem` or `gdstem_fire`; the `biogeography` set has a different community numbering and the model stops on it |
| `tables/` | PFT climate limits and names |
| `static/`, `examples/west/` | western example: forcing, static fields, land cover, CO2, global lightning and population |
| `examples/era5_conus/`, `tools/` | ERA5 example configuration and download tools |
| `technical_desc.md` | inputs, outputs, PFT selection, migration |
| `config_desc.md` | `tem_config.yaml` key reference |
| `flowchart.md` | call hierarchy |

`io` and `driver` may include from `physics`. `physics` includes nothing from `io` or `driver`, so that the science can be built and tested without YAML, NetCDF or MPI. The rule is checked by `.github/scripts/check_layering.py`; today's exceptions are listed in `.github/layering.yaml` and the list may only shrink. See [CONTRIBUTING.md](CONTRIBUTING.md).

## Build

Requires a C++17 compiler, make, libyaml and libnetcdf (C). `make mpi` also requires an MPI C++ compiler and an MPI-enabled netCDF (`netcdf-mpi` with `hpcx-mpi` or `openmpi`; module names vary by site). Build and run with the same MPI.

```bash
conda env create -f env.yaml && conda activate tem-biogeography
make -j4            # build/xtem
make mpi -j4        # build_mpi/xtem
make test
```

Options are passed to `make`, for example `make mpi -j16 OPEN_N=0 STORM=0`. Changing one rebuilds that build directory.

| Option | Default | Effect |
|---|---|---|
| `OPEN_N` | 1 | 1: open nitrogen cycle, 0: closed |
| `FIRE` | 1 | 1: include wildfire, 0: omit it (`veg_fire: 1` then stops with a message) |
| `STORM` | 1 | 1: include storm and hurricane damage (`storm_disturbance: true`, `storm_file`). 0: omit it and reject `storm_disturbance: true` |
| `CXXFLAGS`, `CXX` | `-O2`, `g++` | `make mpi` uses `mpicxx`. An unoptimised build gives different numbers |

There is one executable, `xtem`. Wildfire, N fertilizer and transient wind are keys of `tem_config.yaml`.

## Run

```bash
export TEM_ROOT="$PWD"
mkdir -p ../west_case && cp examples/west/run_config.yaml ../west_case/ && cd ../west_case
"$TEM_ROOT/case_submit.sh"
```

The western example has 374 land cells on a 0.5 degree grid and monthly CCSM4 RCP8.5 forcing for 2015-2099, with dynamic equilibration and wildfire off (`veg_fire: 1` needs `gdp_file` and the `params/gdstem_fire` files). Maps use the Natural Earth state boundaries that Cartopy downloads; `plot_biogeography: false` skips them.

- `n_cpus: 2` runs `build_mpi/xtem` with `mpirun -n 2`. `run_mode: slurm` submits with sbatch (`n_nodes`, `n_cpus`, `walltime`).
- `run_model: false` or `preprocess_only: true` only stages the case. `run_only: true` runs staged inputs. `convert_only: true` rebuilds the derived TEM products. `biogeo_only: true` rebuilds them and the PFT selection. Use at most one.
- `start_year`, `run_length` and `run_length_units` (`days`, `months`, `years`) set the run. The PFT workflow needs whole years and one monthly output file; for anything else set `run_biogeography: false`, which gives a TEM-only run with split output.
- Cohorts of a cell do not interact, and the land-cover cohorts may change between years or be stored ragged (`technical_desc.md`). `tlulcflag: 1` needs a land-cover file with a time axis. The output has `ncohort(time, lat, lon)`, the number of cohorts of each cell and year.
- Under MPI with two or more ranks, rank 0 runs no cells: it reads and checks the inputs, writes the log and writes the output. Ranks 1 to N-1 run the cells, dealt in turn, and send the results to rank 0 with non-blocking sends (`src/io/diag_manager.cpp`, after the FMS diag manager). Rank 0 writes the file a serial run would produce; there are no per-rank files and no merge at the end. Inputs are read in tiles that follow the chunking of the file (`num_io_buffers`, `io_buffer_mb`); at the start rank 0 reads the tiles for the check of the inputs and broadcasts them (`netcdf_read_broadcast`); see `config_desc.md`. Restart files work the same way and can be read with any rank count.

The model writes two files per run, with the same id in both names: `tem_<id>.out` is the progress (the time, the cell each rank is working on with its longitude and latitude, the cells finished and the run time) and `tem_<id>.log` is everything else (setup, grid, cohorts, warnings, errors). The id is `TEM_RUN_ID` when that variable is set, else the start date and time (`YYYYMMDD_HHMMSS`). All ranks append whole lines to the same two files, as FMS does with one run log: rank 0 writes the setup and summary, every rank writes its own progress, and each line is `date time | rank | text` (`.log` lines also have a level: INFO, WARN, ERROR). The first `grdcnt` cells of each rank and every 100th cell after that get a progress line. An error is reported once, by the rank that found it (rank 0 for input errors), in the `.log` (and on stderr at a terminal), and then every rank stops: a rank that exits while MPI is running aborts the others, so no run waits for a rank that has gone. stdout is empty. A batch script should set `TEM_RUN_ID` and append the output of `srun` or `mpirun` to `tem_$TEM_RUN_ID.log` (`>> tem_$TEM_RUN_ID.log 2>&1`), so that MPI messages and crashes land there too. If every cell is masked or an equilibrium fails, the log says why. The model does not interpolate: every input must be on the grid of the land-cover file, and TEM_UW_APP's driver regrids the inputs it stages.

## Inputs

The launcher writes `INPUT/climate.nc`, `static.nc`, `lulc.nc`, `co2.nc` and, if given, `flash.nc`, `popdens.nc`, `gdp.nc`, `storm.nc` and `nfert.nc`, all on the grid of `lulc.nc`. File keys accept paths, globs or lists. A cell runs where `lulc.nc` has a cohort, and cells with missing required data are skipped. Variable names are resolved through `varmap`. Layouts, units and keys are in [technical_desc.md](technical_desc.md#inputs).

Site calibration files are top-level `*_dat` keys. The label is free and the community ID comes from the file. A case that sets any of them replaces the whole default set:

```yaml
hvc_dat: "{tem_src}/params/biogeography/hvc45_openn_dndc.yaml"
hvd_dat: "{tem_src}/params/biogeography/hvd45_openn_dndc.yaml"
```

The [ERA5 CONUS example](examples/era5_conus/run_config.yaml) needs inputs that you supply. Prepare the forcing with the tools in `tools/`; build static, cohort and CO2 data with TEM_UW_APP.

```bash
python tools/get_era5.py --years 1980 2020 --area 50 -125 24 -66 --out /data/era5
python tools/make_input_era5.py --monthly '/data/era5/era5_monthly_*.nc' \
  --daily '/data/era5/era5_t2m_daily_*.nc' --elev /data/elev.nc --out /data/era5_conus.nc
```

## Output

`OUTPUT/tem.nc` is the native monthly output, and `ostateflag` with `ostatefile` writes a NetCDF restart. Unless `run_biogeography: false`, the case builder also writes annual, monthly, fire and PFT products and maps to `OUTPUT/`. The selection rules and products are described in [technical_desc.md](technical_desc.md#pft-selection-and-derived-products).

## Limits

The ERA5 example, SLURM runs and the full western domain have not been run with this version.
