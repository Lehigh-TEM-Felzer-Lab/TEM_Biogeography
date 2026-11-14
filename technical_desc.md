# TEM Biogeography technical description

TEM Biogeography runs the XTEM version 4.5 core (`src/`, the same files as [XTEM](https://github.com/Lehigh-TEM-Felzer-Lab/XTEM)) for the land-cover cohorts of each grid cell and selects potential vegetation from the annual net primary production (NPP) of the cohorts and the climate limits of the plant functional types (PFT). The case builder and launchers are those of [TEM_UW_APP](https://github.com/Lehigh-TEM-Felzer-Lab/TEM_UW_APP). Build and run steps are in [README.md](README.md), the call hierarchy in [flowchart.md](flowchart.md) and the keys of `tem_config.yaml` in [config_desc.md](config_desc.md).

# Case workflow

`case_submit.sh` reads `run_config.yaml` of the case directory, stages the NetCDF inputs in `INPUT/`, writes `tem_config.yaml`, runs `xtem` and then the PFT selection. All case keys with defaults are in `config/run_config.yaml`. The case builder (`src/`) puts every input on the grid of `lulc.nc`, so `model_resolution` stays unset in the generated `tem_config.yaml`.

# Configuration

`tem_config.yaml` is one flat list of keys. The comment above each key in `templates/tem_config.yaml` gives its type and meaning. `run_config.yaml` is a different file: TEM_UW_APP and TEM_Biogeography read it and write `tem_config.yaml`.

| Group | Keys |
|---|---|
| NetCDF inputs | `climate_file`, `static_file`, `lulc_file`, `co2_file`, `lightning_file`, `population_file`, `storm_file`, `nfert_file` (each a path, a list or a glob), `varmap`. A model input is read from the variable of its default name in its group's files; `varmap` (flat, key: variable name in the dataset) is needed only when the dataset defines that variable under another name, and a variable found in neither place stops the run. `mask` is an optional varmap entry |
| Input options | `cldflag`, `parflag`, `xt*flag`, `xtco2flag`, `xto3flag`, `xtndepflag`, `xtws10flag`, `xnfertflag`, `tlulcflag`, `lulclastyr`, `initco2`, `xco2level` |
| Run control | `run_dt`, `run_dt_units`, `run_length`, `run_length_units`, `clmstartyr`, `xequil`, `region`, `mxnumgrid`, `grdcnt`, `strtflag`, `numskip`, `endflag`, `numgrids` |
| Initialization and equilibrium | `spinflag`, `numspin`, `spintime`, `de_startyear`, `de_nyears`, `equil_*`, `wtol`, `ctol`, `ntol`, `inittol`, `maxit`, `maxitmon`, `istateflag`, `istatefile`, `istateyear` |
| Processes | `sradflag`, `avlnflag`, `nfeed`, `baseline`, `moistlim`, `o3flag`, `dc2n`, `ode_solver`, `rsoil_method`, `rhmoist_method`, `agtillflag`, `agirrgflag`, `agfertflag`, `veg_fire`, `fire_*`, `storm_disturbance` |
| Output and state | `output_file`, `output_new_file_freq`, `output_new_file_freq_units`, `startyr`, `endyr`, `diffyr`, `temoutflag`, `temoutvars`, `clmoutflag`, `clmoutvars`, `spinoutflag`, `spinoutyrs`, `ostateflag`, `ostatefile`, `ostateyear` |
| Parameter files | `cmntecd`, `rootzecd`, `vegecd`, `conductecd`, `mcrbecd`, `agecd` |
| Site calibration files | `*_dat` |

## Time step and run length

All times are converted to seconds on the 365-day noleap year (`src/physics/tem_time.cpp`, after the time manager of FMS). `run_dt` and `run_length` take a count and a unit: `second`, `minute`, `hour`, `day`, `week`, `month` or `year` (also `s`, `sec`, `min`, `h`, `hr`, `d`, `yr`). `1 day`, `24 hours` and `86400 seconds` are the same step.

The step is one calendar month (or n months that divide 12) or a whole number of days, at most 28, that divides the year. Any other step stops the run with the reason: shorter than a day (no sub-daily forcing), longer than 28 days and not calendar months, or not dividing the year.

The run starts on 1 January of `clmstartyr`. Its length in seconds must be a whole number of steps, for example `7 days` at a daily step or `17 months` at a monthly step; months are calendar months.

Every input with a time dimension must cover the run: one record per step for step data, per calendar month for monthly data, per year for annual data. A shorter file stops the run, and the message gives the date range of the file and the end of the run. Extra records are ignored, and a one-year file is reused every year. A run that ends inside a year closes that year like a full one; its output equals the first steps of a longer run (tested for 41 months on 1, 2 and 3 ranks). TEM_UW_APP and TEM_Biogeography take `start_year`, `run_length` and `run_length_units` and set `clmstartyr`, `startyr` and `endyr` from them.

## Options

All are off by default; with them off the output is unchanged.

**N fertilizer.** As in GDSTEM, fertilizer comes only from annual input data: `xnfertflag: 1` reads one rate per cell for crops and pasture (`nfertcrop`, `nfertpasture` in `nfert_file`), `2` reads rates for every year, and there is no built-in series. The data are read when `agfertflag` is 1. The rate is divided by 12000 as in land_legacy; its units are not documented in that code.

**Fire.** `veg_fire: 1` turns fire on; it works at any `run_dt`. The fire model is GDSTEM-FIRE: burned area from the Li et al. (2012) scheme as in CLM5, with ignitions from lightning, population density and the ignition efficiency `igef` of the site file, and fuel from the carbon pools, relative humidity, soil wetness, GDP and population suppression (`fsr`, `fd`). Use the parameter files of `params/gdstem_fire`. The burned fraction of a cohort is the output `areaburn` (fraction, 1); prescribed fires (`disturbflag 3`) burn the whole cohort. Fire runs in the Biogeography model only when the core is built with the `FIRE` option.




**Storms.** `storm_disturbance: true` (build option `STORM=1`) draws tropical storm and hurricane damage from the return intervals in `storm_file`.

**Competition.** There is no `competition` key. Cohorts of a cell do not interact, and the two loop orders of the earlier key gave identical results (checked bit for bit). A configuration that sets `competition` to 0 gets a warning and to 1 an error.

**Fire build option.** `make FIRE=0` leaves the wildfire code out of the run (`TEM_ENABLE_FIRE`). The default is 1.

**Community numbers.** The code tests some communities by number (ice 1, tall grassland 9, and so on, as in GDSTEM). When the vegetation file is read, the description of each of communities 1 to 14 is compared with that role (`tem_roles.cpp`) and the run stops on a mismatch, because a parameter set with other numbers would run and give wrong results. `params/gdstem` and `params/gdstem_fire` have this numbering; `params/biogeography` and `params/glstem` do not.

## Land-cover cohorts

`tlulcflag: 1` (cohorts that change by year) needs a land-cover file with a time axis, one record per year starting at `clmstartyr` and reaching `lulclastyr`; without one the run stops with an error. `tlulcflag: 0` uses a file without a time axis. The land-cover file (`lulc_file`) holds, for every cell and year, the cohorts and their `standage`, `chrtarea`, `potveg`, `currentveg`, `subtype`, `agstate`, `agprevstate`, `disturbflag` and, optional, `isrccohort`. The cohorts of a cell keep their index from year to year and new ones are appended. A cohort that appears in a year is started from the state of cohort `isrccohort` (1-based, an existing one); without it the run stops, because a copy of itself would be uninitialised. Count and cohorts may change every year.

Two layouts are read. The dense layout is a cube `(time, cohort, lat, lon)` with a fill value where a cohort does not exist; the cohorts of a cell and year must be the first ones. The ragged layout follows the CF contiguous ragged array: `ncohort(time, lat, lon)` with the attribute `sample_dimension = "obs"` and optional `max_cohort`, and every variable on the single dimension `obs`, ordered by longitude, latitude, time, cohort. Nothing is stored for a cohort that does not exist, so the file grows with the cohorts that exist and not with cells x years x the largest count. A longitude column is read at a time. Output is not accumulated: each cohort writes its steps of the year when the year ends, so a cohort holds one year of output, and only the inputs of the cell being run are held for all its years.

The output cohort dimension is unlimited and starts at the largest count of the input (the cohort axis of a dense file, or `max_cohort` of a ragged one); a higher cohort index extends it, so a cohort count that changes from year to year needs nothing fixed beforehand. The variables the run writes (`clmoutvars`, `temoutvars` and the eight cohort variables) are defined once when a file is created; a variable that was not declared is added when its first record arrives. `ncohort(time, lat, lon)` holds the number of cohorts of the cell in the year (cohorts 1 to `ncohort` have data; 0 where there is none), constant over the steps of a year; a cohort that does not exist keeps the fill value, which costs no storage when a whole chunk is unwritten and almost none otherwise. `time` and `time_bnds` of a year are written once. A record is placed by index: the position on each axis is computed from the grid and checked against it (no search for the nearest value), the time position follows from the year and the first year of the file, and a file holds whole years (`output_new_file_freq` must be a multiple of 12 months), so a year is never split between files. The first year of the files is the start of the run, not the first record to arrive, as the base time of an FMS file is set at its initialisation; earlier records are counted and reported, and a record written twice stops the run. With two or more ranks, rank 0 runs no cells (I/O and orchestration only, `part_rank` -1) and ranks 1 to N-1 compute; a run with one rank or the serial build computes and writes. Only rank 0 touches the file (compute ranks collect 256 KiB batches of records, at most 4 sent and not yet received, the root keeps one receive of 512 KiB posted per compute rank; FMS sends each field once with blocking calls and has no such sizes, and 64 KiB to 16 MiB took the same time in the transport test): it takes the records of the compute ranks from `diag_manager` (non-blocking sends; rank 0 has non-blocking receives posted and writes the records as they arrive; with one rank it writes directly). The chunks are (all steps of a year, 1 cohort, 16 x 16 cells) and rank 0 caches 128 MB per variable, so a chunk is compressed once when it is evicted, not at each cell written into it.

## Parameter files

Vegetation, soil and site parameters are YAML in `params/`, consolidated from GDSTEM and TEM_Biogeography. Six top-level keys name the community files (`cmntecd`, `rootzecd`, `vegecd`, `conductecd`, `mcrbecd`, `agecd`). A site calibration is a top-level key ending in `_dat`, with any label:

```yaml
ice_dat: ../../params/gdstem/ice45_openn_303.yaml
tpd_dat: ../../params/gdstem/tpd45_trendy.yaml
tmt_dat: ../../params/gdstem/tmt45_trendy.yaml
bnz_dat: ../../params/gdstem/bnz45_trendy.yaml
hvc_dat: ../../params/gdstem/hvc45_trendy.yaml
hvd_dat: ../../params/gdstem/hvd45_trendy.yaml
tai_dat: ../../params/gdstem/tai45_trendy.yaml
pwn_dat: ../../params/gdstem/pwn45_trendy.yaml
knz_ag0_dat: ../../params/gdstem/knz45_ag0_trendy.yaml
gum_dat: ../../params/gdstem/gum45_trendy.yaml
cur_dat: ../../params/gdstem/cur45_trendy.yaml
gux_dat: ../../params/gdstem/gux45_trendy.yaml
duc_evergreen_dat: ../../params/gdstem/duc45_evergreen_trendy.yaml
duc_deciduous_dat: ../../params/gdstem/duc45_deciduous_trendy.yaml
impervious_dat: ../../params/gdstem/impervious_trendy.yaml
turf_dat: ../../params/gdstem/turf45_trendy.yaml
kbs_dat: ../../params/gdstem/kbs45_trendy.yaml
kbm_dat: ../../params/gdstem/kbm45_trendy.yaml
osg_dat: ../../params/gdstem/osg45_trendy.yaml
knz_ag2_dat: ../../params/gdstem/knz45_ag2_trendy.yaml
pwn_ag2_dat: ../../params/gdstem/pwn45_ag2_trendy.yaml
```

TEM reads `community[].type` of each file and uses that ID to assign the calibration. Each community ID can appear once. Use mosaic and parameter files from the same calibration set.

# Inputs

## Files and variables

Input keys take a NetCDF path, a glob, or a list of both. Any extension works (`.nc`, `.nc4`, `.cdf`, none); libnetcdf detects the format, and wildcards skip files it does not recognise. A model input is read from the variable of its default name in the NetCDF dataset; `varmap` (key: variable name in the dataset) is needed only when the dataset defines that variable under another name. The keys are the default names of the climate, static, `co2`, `gdp`, `nfertcrop` and `nfertpasture` variables, and `lightning` (default `lightning_flashes`), `popdens` (`population_density`), `storm` (`storm_return_interval`) and `hurricane` (`hurricane_return_interval`). `mask` has no default: with a `mask` entry, cells where that static variable is 0 or below do not run. The land-cover fields are read by name. A `varmap` value is one variable name, no sums and no options, for example:

```yaml
varmap:
  tair: t2m
  wind: wind_speed
  sand: sand_pct
  lightning: flash_density
```

Units come from the `units` attribute (a file in Kelvin needs `units: K`); without it the values are taken as given. `clmoutvars` and `temoutvars` are YAML sequences such as `[tair, prec]`; use `[]` for none.

## Grid, time and cells

Each check below stops the run with the file and the values that differ (`src/io/tem_input_check.cpp`).

- **Grid.** `lon` and `lat` are in degrees (or have no units), within -360 to 360 and -90 to 90, regular (spacing within 1 % of the mean, monotonic) and have the same points in every gridded input, within 0.1 % of the cell width. `lon` may be 0..360 or -180..180, `lat` may run either way, and the dimension order inside a variable is free. Dimensions are recognised by name (`lon`, `lat`, `time`) or by CF attributes; other names are extra axes. Projected, radian and Gaussian grids must be regridded first.
- **Time.** The axis increases. Its spacing in seconds is the model step, a calendar month or a year, and is the same throughout. The calendar is `noleap`, `365_day` or `standard` without 29 February; a daily `360_day` file or a year of 366 daily records stops the run. The first record is in `clmstartyr` for every input with a time axis (climate, transient wind, ozone, N deposition, fertilizer, land-cover change). A `month` axis of 12 is read as a climatology.
- **Cells.** A cell runs where the land-cover input has data (a cell without a cohort cannot run) and, when `varmap` has a `mask` entry, where that variable of the static file is above 0. Every other input must have data in every such cell, or the run stops with the count and the first position per input (`_FillValue` or NaN is missing). `mask` in the output is 1 for cells that ran; the log line `grid:` gives the cells run, those without land cover and those outside the mask.

## Grids

The model does not interpolate. The land-cover file (`lulc_file`) defines the model grid and every other gridded input must be on the same longitudes and latitudes, or the run stops with a message that names the file, its grid and the grid of the land cover. Regrid the inputs before the run, for example with `cdo remapcon` (amounts, rates, densities), `cdo remapbil` (smooth state fields) or xesmf; TEM_UW_APP does this for the cases it builds. `co2` has no grid. `model_resolution` is no longer a key and is refused when it is set.

## File layouts

All variable names are lowercase. `lat` (`degrees_north`) and `lon` (`degrees_east`) are in every gridded file. The sample inputs in `examples/test/INPUT` of XTEM use float32 for floats and int64 for categories.

| File (key) | Dimensions | Variable | Unit | Required | Notes |
|---|---|---|---|---|---|
| `climate.nc` | `time`, `lat`, `lon` | `tair` | degC | yes | |
| | | `dtr` (diurnal range) | degC | yes | |
| | | `prec` | mm month-1 | yes | rate per step |
| | | `nirr` (net irradiance) | W m-2 | yes | |
| | | `vpr` (vapor pressure) | hPa | yes | |
| | `time`: `days since <date>`, `noleap` | `time` | | | the first record is in `clmstartyr`; every time-dependent input starts in that year |
| `co2.nc` | `time` | `co2` | ppmv | yes | no `lat`/`lon`; one value per year |
| `static.nc` | `lat`, `lon` | `elev` | m | yes | |
| | | `sand`, `silt`, `clay` | percent | yes | each its own variable |
| | | `ndep` | | yes | `xtndepflag: 1` gives `time` as well |
| | `month`, `lat`, `lon` | `wind` | m s-1 | yes | climatology; `time` instead when `xtws10flag: 1` |
| | | `ozone` | AOT40 | yes | climatology |
| `lulc.nc` | `cohort`, `lat`, `lon` (`cohort` is also an int64 variable, 1 to N) | `standage`, `chrtarea`, `potveg`, `currentveg`, `subtype` | | yes | a cohort exists where `standage` has data |
| | | `agstate`, `agprevstate`, `tillflag`, `fertflag`, `irrgflag`, `disturbflag`, `disturbmonth`, `FRI`, `slashpar`, `vconvert`, `prod10par`, `prod100par`, `vrespar`, `sconvert` | | no | default 0 (`FRI` 2000); `(time, cohort, lat, lon)` gives yearly land-cover change |
| `flash.nc` | `time` or `month`, `lat`, `lon` | `lightning_flashes` | km-2 per step | `veg_fire: 1` | one value per step of the year (12 monthly) |
| `popdens.nc` | `lat`, `lon` | `population_density` | km-2 | `veg_fire: 1` | |
| `gdp.nc` | `lat`, `lon` | `gdp` | thousand US dollars per person | `veg_fire: 1` | GDP suppression of fire, see Fire inputs |
| `storm.nc` | `lat`, `lon` | `storm_return_interval`, `hurricane_return_interval` | years | `storm_disturbance: true` | zero disables an event; positive intervals are finite and at least 1; `varmap` keys `storm` and `hurricane` name other variables |
| nfert files | `time`, `lat`, `lon` | `nfertcrop`, `nfertpasture` | | `agfertflag: 1` | annual rates; the example holds one rate per cell (`xnfertflag: 1`) |

The number of cohorts of a cell and year is counted from `standage` of `lulc.nc`; there is no separate count file. `cohort`, `potveg`, `currentveg` and `agstate` carry `flag_values` and `flag_meanings` listing the TEM vegetation types by name.

## Fire inputs

`veg_fire: 1` needs lightning, population density and GDP.

| File key | Variable | Dimensions | Units | Needed | Where to get it |
|---|---|---|---|---|---|
| `lightning_file` | `lightning_flashes` | `time` (one record per step of the year, 12 monthly) or `month` (12), `lat`, `lon` | flashes km-2 per step (km-2 month-1 at monthly steps) | `veg_fire: 1` | LIS/OTD 0.5 degree monthly climatology HRMC v2.3.2015, NASA GHRC DAAC (Cecil et al. 2014, https://doi.org/10.1016/j.atmosres.2012.06.028; file `LISOTD_HRMC_V2.3.2015.nc`, variable `HRMC_COM_FR`); that rate is flashes km-2 per day, multiply by the days of the month (for a daily step by the days of the step) |
| `population_file` | `population_density` | `lat`, `lon` | persons km-2 | `veg_fire: 1` | Gridded Population of the World v4.11 population density (CIESIN/SEDAC, https://doi.org/10.7927/H49C6VHW), persons per km2, or WorldPop persons summed in the cell and divided by the cell area |
| `gdp_file` | `gdp` | `lat`, `lon` | thousand US dollars per person | `veg_fire: 1` | Kummu et al. (2018) gridded GDP per capita, https://doi.org/10.5061/dryad.dk1j0 (2011 international dollars; divide by 1000), or the GDP field of the CLM5 surface dataset (CESM input data) regridded to the model grid |

With `gdp_file`, the GDSTEM-FIRE terms need `fsr` (fire spread rate, m s-1), `fd` (fire duration, h), `rswf_min` and `rswf_max` (soil wetness limits) for each community in `vegecd`. They are in `params/biogeography` and `params/gdstem` (taken by community name from `params/gdstem_fire`). A community without them stops the run when `gdp_file` is given. Without `gdp_file` none of these inputs or parameters is read.

Not taken from GDSTEM-FIRE: its pool-specific combustion constants (the XTEM combustion fractions apply) and one GDP and population field per year (one field is read). GDSTEM-FIRE has no species emissions; the carbon and nitrogen lost in a fire (`volac`, `volan`, standing dead, CWD, FWD) are fluxes XTEM already has. Burned area can be compared with GFED4s (van der Werf et al. 2017), which is not an input.

## Sources

TEM_UW_APP builds these files (its `src/gen_climate_inputs.py` for forcing, its `src/gen_static_inputs.py` for the rest); download commands are in its `technical_desc.md`.

| Input | Source |
|---|---|
| Temperature, precipitation, radiation, vapor pressure | ERA5 (Hersbach et al. 2020); MACAv2-METDATA (Abatzoglou and Brown 2012) and gridMET (Abatzoglou 2013) for CONUS; CRU TS (Harris et al. 2020); CMIP5/6 (Taylor et al. 2012; Eyring et al. 2016) |
| CO2 | Meinshausen et al. 2020 for scenarios; NOAA GML for observed years |
| N deposition | ISIMIP3b simulated historical deposition (Yang et al. 2026); Lamarque et al. 2013 |
| Ozone (AOT40) | gridded surface ozone metrics (Evans and Sofen 2016; Sofen et al. 2016) |
| Soil texture, elevation, wind | SoilGrids 2.0 (Poggio et al. 2021); ETOPO 2022 (NOAA NCEI 2022); WorldClim 2.1 (Fick and Hijmans 2017) |
| Land cover | LUH2 (Hurtt et al. 2020); potential vegetation of Ramankutty and Foley (1999) |
| Lightning (`flash.nc`) | LIS/OTD HRMC 0.5 degree monthly climatology v2.3.2015 (Cecil et al. 2014) |
| Population density (`popdens.nc`) | GPWv4.11 (CIESIN 2018) or WorldPop 2020 (WorldPop 2018) |
| GDP per person (`gdp.nc`, optional) | Kummu et al. (2018), or the GDP field of the CLM5 surface dataset (Lawrence et al. 2019) |
| Month of crop burning (`crop.nc`, optional) | agricultural fire peak month of the CLM5 surface dataset (Lawrence et al. 2019) |

Check units before use. ERA5 `t2m` is K, `tp` is m per accumulation period and radiation is J m-2; CMIP `tas` is K and `pr` is kg m-2 s-1. Daily or hourly data must be averaged to months first (TEM_UW_APP does this). The variable names of the CLM5 surface dataset are those known from CLM; check them in the file you download.

## References

- Abatzoglou, J. T. (2013) Int. J. Climatol. 33, 121-131. https://doi.org/10.1002/joc.3413
- Abatzoglou, J. T. and Brown, T. J. (2012) Int. J. Climatol. 32, 772-780. https://doi.org/10.1002/joc.2312
- Cecil, D. J., Buechler, D. E. and Blakeslee, R. J. (2014) Atmos. Res. 135-136, 404-414. https://doi.org/10.1016/j.atmosres.2012.06.028
- CIESIN (2018) Gridded Population of the World, Version 4 (GPWv4): Population Density, Revision 11. https://doi.org/10.7927/H49C6VHW
- Evans, M. J. and Sofen, E. D. (2016) Gridded Global Surface Ozone Metrics data (1971-2015), version 2.7. CEDA. https://doi.org/10.5285/897a3958-5bfc-4311-9bb0-01134bf6aefa
- Eyring, V. et al. (2016) Geosci. Model Dev. 9, 1937-1958. https://doi.org/10.5194/gmd-9-1937-2016
- Fick, S. E. and Hijmans, R. J. (2017) Int. J. Climatol. 37, 4302-4315. https://doi.org/10.1002/joc.5086
- Harris, I. et al. (2020) Sci. Data 7, 109. https://doi.org/10.1038/s41597-020-0453-3
- Hersbach, H. et al. (2020) Q. J. R. Meteorol. Soc. 146, 1999-2049. https://doi.org/10.1002/qj.3803
- Hurtt, G. C. et al. (2020) Geosci. Model Dev. 13, 5425-5464. https://doi.org/10.5194/gmd-13-5425-2020
- Kummu, M., Taka, M. and Guillaume, J. H. A. (2018) Sci. Data 5, 180004. https://doi.org/10.1038/sdata.2018.4
- Lamarque, J.-F. et al. (2013) Atmos. Chem. Phys. 13, 7997-8018. https://doi.org/10.5194/acp-13-7997-2013
- Lawrence, D. M. et al. (2019) J. Adv. Model. Earth Syst. 11, 4245-4287. https://doi.org/10.1029/2018MS001583
- Li, F. and Lawrence, D. M. (2017) J. Clim. 30, 1893-1908. https://doi.org/10.1175/JCLI-D-16-0556.1
- Li, F., Zeng, X. B. and Levis, S. (2012) Biogeosciences 9, 2761-2780. https://doi.org/10.5194/bg-9-2761-2012
- Meinshausen, M. et al. (2020) Geosci. Model Dev. 13, 3571-3605. https://doi.org/10.5194/gmd-13-3571-2020
- NOAA NCEI (2022) ETOPO 2022 15 Arc-Second Global Relief Model. https://doi.org/10.25921/fd45-gt74
- Poggio, L. et al. (2021) SOIL 7, 217-240. https://doi.org/10.5194/soil-7-217-2021
- Ramankutty, N. and Foley, J. A. (1999) Global Biogeochem. Cycles 13, 997-1027. https://doi.org/10.1029/1999GB900046
- Sofen, E. D. et al. (2016) Earth Syst. Sci. Data 8, 41-59. https://doi.org/10.5194/essd-8-41-2016
- Taylor, K. E., Stouffer, R. J. and Meehl, G. A. (2012) Bull. Am. Meteorol. Soc. 93, 485-498. https://doi.org/10.1175/BAMS-D-11-00094.1
- van der Werf, G. R. et al. (2017) Earth Syst. Sci. Data 9, 697-720. https://doi.org/10.5194/essd-9-697-2017
- WorldPop (www.worldpop.org) and CIESIN, Columbia University (2018) Global High Resolution Population Denominators Project. University of Southampton. https://doi.org/10.5258/SOTON/WP00647
- Yang, J., Tian, H. and Büchner, M. (2026) ISIMIP3b Simulated Historical Nitrogen Deposition Input Data, v1.1. ISIMIP Repository. https://doi.org/10.48364/ISIMIP.600567.1

# Outputs

## Output file

One file per run, `output_file`, or `<stem>_<YYYYMM>-<YYYYMM><extension>` when `output_new_file_freq` is set. Under MPI the ranks send their records to rank 0, which writes the file (`diag_manager.h`); there are no per-rank files. Global attributes: `title`, `source` (`XTEM`), `region`, `Conventions` (`CF-1.8`). The file is NetCDF-4 and each variable is chunked as (all time steps, 1 cohort, up to `output_chunk_lonlat` latitudes and longitudes, default 16), so a time series of one cell reads fast and each cell is written in one pass; with `output_chunk_lonlat: 0` NetCDF chooses the chunks, as FMS does when no chunk sizes are given (one map per step: 117 s instead of 12 s on the 10-cell global test, 13 times larger file, because the cells are written one after the other); `xarray.open_dataset`, `ncdump`, `cdo` and Panoply read it.

| Name | Dimensions | Type | Content |
|---|---|---|---|
| `time` | `time` | double | `days since 1970-01-01 00:00:00`, `noleap`, `axis: T`, bounds `time_bnds`; one record per model step (12 per year monthly, 365 daily) |
| `time_bnds` | `time`, `bnds` | double | start and end of each step |
| `cohort` | `cohort` | int64 | 1 to the number of cohorts; also the dimension of the cohort variables |
| `lat`, `lon` | | float32 | CF `standard_name` set |
| `mask` | `lat`, `lon` | int64 | 1 where TEM ran the cell, 0 where inputs were missing or masked |
| TEM variables of `temoutvars` (for example `gpp`, `pet`, `potevap`, `actevap`, `areaburn`) | `time`, `cohort`, `lat`, `lon` | float | `long_name` and `units` from the registry in `src/io/tem_outvars.cpp`; the unit `month-1` becomes `day-1` (365 or more steps) or `step-1` for other steps; missing cells are `_FillValue` |
| `standage`, `subtype`, `cmnt`, `currentveg`, `potveg`, `agstate` | `time`, `cohort`, `lat`, `lon` | int32 | cohort metadata of the year, repeated for every step |
| `carea`, `subarea` | `time`, `cohort`, `lat`, `lon` | double | cell and cohort area, km2 |
| Climate variables of `clmoutvars` (for example `tair`) | `time`, `lat`, `lon` | float | one value per cell |

## Variable registry

Names, long names, units and storage type of the output variables are registered in C++, as FMS registers diagnostic fields: the table is in `src/io/tem_outvars.cpp`. To add one, add a line there or call `tem::nc::registerOutput( name, long_name, units, kind )` (`Float32` by default, `Float64`, `Int32`, `Int64`) before the output is configured. Floats are written as float32. `time` and `time_bnds` are float64, since days since 1970 need the range, and the state file keeps float64 so a restart reproduces the run exactly.

## State file

`ostatefile` is a separate NetCDF file for restarts. It holds the model state of every cohort of every cell at equilibrium (`ostateflag: 1`) or at the end of `ostateyear` (`ostateflag: 2`); `istatefile` with `istateflag` 1 or 2 reads it back and skips the equilibration.

| Name | Dimensions | Type | Content |
|---|---|---|---|
| `record` | unlimited | | one record per cell and cohort |
| `item` | number of state values | | order fixed by `writeCohortState` in `telm_gds.cpp` |
| `lon`, `lat` | `record` | double | cell centre (degrees_east, degrees_north) |
| `cohort` | `record` | int | cohort number within the cell, from 1 |
| `state` | `record`, `item` | double | pools, fluxes and running means of the cohort |

Under MPI the compute ranks send their records to rank 0 through `diag_manager`, the transport of the output, and rank 0 writes the one file; there are no files per rank and no merge at the end. Records are looked up by (lon, lat, cohort), so any number of ranks can read a file written by any other number. A restart reproduces the writing run exactly (checked on 50 cells, serial and 2 ranks). The file fits only the grid and parameter files it was written with.

# Source files

All C++ sources are in `src/`: `physics/` (science), `io/` (files, MPI, log) and `driver/` (main program). `physics` does not include from `io` or `driver` (`.github/scripts/check_layering.py`). The first column names the directory. The call hierarchy, with the file of each function, is in [flowchart.md](flowchart.md).

| Files | Content |
|:-------------|:------------------------------------------------------|
| `driver/xtem_gds.cpp` | driver of xtem: reads the configuration and inputs, loops over cells and cohorts, writes output and state |
| `physics/telm_gds.cpp`, `physics/telm_gds.h` | grid cell of xtem: ties climate, land cover and the ecosystem model together |
| `physics/telmntcohort_gds.hpp` | land-cover cohort of a cell |
| `physics/ttem_gds.cpp`, `physics/ttem_gds.h` | carbon, nitrogen and water cycles of one cohort and their integration in time |
| `physics/tveg_gds.cpp`, `physics/tveg_gds.h` | vegetation: photosynthesis, allocation, phenology, litterfall |
| `physics/tsoil_gds.cpp`, `physics/tsoil_gds.h` | soil: texture, water and root zone |
| `physics/tmcrb_gds.cpp`, `physics/tmcrb_gds.h` | soil microbes: decomposition and nitrogen mineralization |
| `physics/penmon_gds.cpp`, `physics/penmon_gds.h` | evapotranspiration (Penman-Monteith, Shuttleworth-Wallace) |
| `physics/humnact_gds.cpp`, `physics/humnact_gds.h` | land conversion, agriculture and standing dead wood |
| `physics/fire.cpp`, `physics/fire.h` | wildfire occurrence and burned fractions |
| `physics/tbiome_gds.cpp`, `physics/tbiome_gds.h` | vegetation mosaic of a cell |
| `physics/tlcluc_gds.cpp`, `physics/tlcluc_gds.h` | potential and current land cover of a cell |
| `physics/tclm_gds.cpp`, `physics/tclm_gds.h` | climate, with ozone, nitrogen deposition and wind |
| `physics/atms_gds.cpp`, `physics/atms_gds.h` | atmosphere: CO2, ozone, nitrogen deposition |
| `physics/tclmdat_gds.cpp`, `physics/tclmdat_gds.h` | climate record of a cell |
| `physics/lulcdat_gds.cpp`, `physics/lulcdat_gds.h` | land-cover record of a cell |
| `physics/atmdepdat_gds.cpp`, `physics/atmdepdat_gds.h` | nitrogen deposition record of a cell |
| `physics/nfertdat_gds.cpp`, `physics/nfertdat_gds.h` | nitrogen fertilizer record of a cell |
| `physics/telvdat_gds.cpp`, `physics/telvdat_gds.h` | elevation record of a cell |
| `physics/tsoldat_gds.cpp`, `physics/tsoldat_gds.h` | soil texture record of a cell |
| `physics/tmaxcohortdat_gds.cpp`, `physics/tmaxcohortdat_gds.h` | maximum number of cohorts of a cell |
| `physics/bioms_gds.hpp` | carbon and nitrogen components of biomass |
| `physics/elmnt_gds.cpp`, `physics/elmnt_gds.h` | position and counters of a grid cell |
| `physics/ttimestep.h` | calendar of the model step (run\_dt) |
| `physics/temconsts_gds.hpp` | constants and the compile-time options |
| `io/tem_yaml.cpp`, `io/tem_yaml.h` | YAML reader of the configuration and parameter files |
| `io/tem_invars.cpp`, `io/tem_invars.h` | which file and variable supplies each model input (varmap) |
| `physics/tem_time.cpp`, `physics/tem_time.h` | time in seconds: unit conversion, CF time units, noleap calendar |
| `io/tem_input_check.cpp`, `io/tem_input_check.h` | checks that the grids, time axes and cells of all inputs match |
| `io/tem_netcdf_io.cpp`, `io/tem_netcdf_io.h` | NetCDF input and output, grid, masks and partition of the cells |
| `io/tem_outvars.cpp`, `io/tem_outvars.h` | registry of the output variables with names, long names and units |
| `io/tem_state_io.cpp`, `io/tem_state_io.h` | restart state file |
| `io/mpi_layer.h` | interface to the parallel runtime |
| `io/mpi_serial.cpp` | serial implementation of the interface (default) |
| `io/mpi_parallel.cpp` | MPI implementation of the interface (make mpi) |

# PFT selection and derived products

The native monthly output `OUTPUT/tem.nc` is described under Outputs. The case builder converts it to these products:

| Derived product in `OUTPUT/` | Content |
|---|---|
| `tem_annual_*.nc` | annual flux sums, mean rates and states, end-of-year cohort metadata |
| `tem_monthly_*.nc` | selected monthly cohort diagnostics |
| `tem_mmdi_monthly_*.nc` | AET, PET, AET/PET and 1 - AET/PET where PET is positive |
| `tem_fire_monthly_*.nc` | fraction of each cohort burned (`areaburn`), with wildfire on |
| `biogeo_annual_*.nc` | winning PFT per year, its diagnostics, climate suitability, persistent PFT per period |
| `summary_annual_*.nc` | PFT and regional statistics by year, area-weighted means |
| `potveg_periods_*.png` | maps of the most frequent annual winner per period |

The selector computes growing degree days, the coldest and warmest monthly mean temperature and annual precipitation from the monthly climate, and the mean AET/PET of `summer_months`
(valid monthly records pooled by PFT and subtype in each cell and year). Among cohorts that meet all limits the highest annual NPP wins; if none does, the highest NPP overall wins; a tie goes to the lowest cohort index.
The persistent PFT of a period is its most frequent annual winner, ties broken by higher summed NPP, then lower PFT code.

# Migration and checks

TEM Biogeography combines the Biogeography workflow (PFT rules, western inputs), the XTEM core (`src/`, identical to XTEM) and the case builder and launchers of TEM_UW_APP.

## Inputs

- The western CCSM4 RCP8.5 forcing keeps its original grid and fields on a noleap monthly axis; wind and elevation are in `static.nc`. The 2014 ozone records form a 12-month climatology, and nitrogen deposition keeps its original values and convention.
- Cohort order, subareas and vegetation codes come from the original cohort table; the cohort count is taken from `standage` in `lulc.nc`. Grid-cell area uses the XTEM calculation.
- Lightning and population grids encode missing source values as zero, as the earlier fire preprocessor did.
- The text fire diagnostics and relative soil wetness of the old workflow are replaced by the native `areaburn`, `potevap` and `actevap`.

## What is not shown

The core is the XTEM core, not TEM 4.5, so numbers are not expected to equal TEM 4.5. Degree days use the staged noleap climate with the same PFT limits as the earlier workflow.

## Checks

- Western example (11 x 7 cells, 2015-2018, fire on): the native, derived and PFT files are identical to the build before this version.
- `make test` passes.
- The ERA5 example needs external forcing and cluster runs depend on the local SLURM and MPI; neither was run. The full western domain (374 cells, 2015-2099) was not run to completion.
