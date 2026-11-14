# tem_nc.py
"""Common NetCDF metadata and writing for all TEM workflow outputs."""

import datetime
import logging
from pathlib import Path

import cftime
import xarray as xr
import yaml

from tem_config import ROOT

log = logging.getLogger("NETCDF")


def table(name: str) -> dict:
    """NetCDF variable descriptions by section."""
    return yaml.safe_load((ROOT / "tables" / "nc_variables.yaml").read_text())[name]


def open_annual(path: Path) -> xr.Dataset:
    """Load a derived annual NetCDF file with an integer year dimension."""
    with xr.open_dataset(path) as ds:
        return (
            ds.assign_coords(year=ds.time.dt.year)
            .swap_dims(time="year")
            .drop_vars("time")
            .load()
        )


def describe(ds: xr.Dataset, section: str) -> xr.Dataset:
    """Set long_name and units of the data variables listed in the NetCDF metadata table."""
    for name, attrs in table(section).items():
        if name in ds:
            ds[name].attrs = {**attrs, **ds[name].attrs}
    return ds


# kind -> (file name stem, time step); the files are named like climate model output:
# <product>_<frequency>_<case>_<first>-<last>.nc, with YYYY for annual and YYYYMM for monthly data
KINDS = {
    "tem_annual": ("tem_annual", "year"),
    "tem_monthly": ("tem_monthly", "month"),
    "mmdi": ("tem_mmdi_monthly", "month"),
    "fire": ("tem_fire_monthly", "month"),
    "biogeo": ("biogeo_annual", "year"),
    "summary": ("summary_annual", "year"),
    "potveg_periods": ("potveg_periods", "year"),
}


def output_path(cfg: dict, kind: str, suffix: str = ".nc") -> Path:
    """OUTPUT/<product>_<case>_<period><suffix> of the product named kind."""
    stem, step = KINDS[kind]
    y0, y1 = cfg["start_year"], cfg["end_year"]
    period = f"{y0}-{y1}" if step == "year" else f"{y0}01-{y1}12"
    return cfg["case_dir"] / "OUTPUT" / f"{stem}_{cfg['case_name']}_{period}{suffix}"


def write(
    ds: xr.Dataset,
    kind: str,
    cfg: dict,
    title: str,
    summary: str,
    integers: set[str] = frozenset(),
) -> None:
    """Add coordinate and global attributes (CF-1.8), compress, and write the file of the product kind."""
    path = output_path(cfg, kind)
    time_encoding = {
        "units": "days since 1970-01-01 00:00:00",
        "calendar": "noleap",
        "dtype": "float64",
    }
    if "year" in ds.dims:
        times = [cftime.DatetimeNoLeap(int(year), 1, 1) for year in ds.year.values]
        ds = ds.rename(year="time").assign_coords(time=times)
    ds = ds.transpose("time", ..., "lat", "lon", missing_dims="ignore").drop_encoding()
    for name, attrs in table("coordinates").items():
        if name in ds.coords:
            ds[name].attrs = {**attrs, **ds[name].attrs}
    ds.attrs = {
        "title": title,
        "summary": summary,
        "Conventions": "CF-1.8",
        "institution": "Lehigh University, Felzer Lab",
        "source": "Terrestrial Ecosystem Model (TEM-Hydro) with biogeography module, github.com/Lehigh-TEM-Felzer-Lab/TEM_Biogeography",
        "product": KINDS[kind][0],
        "frequency": KINDS[kind][1],
        "case_name": cfg["case_name"],
        "grid_resolution_degrees": float(cfg["horizontal_resolution"]),
        "simulation_start_year": int(cfg["start_year"]),
        "simulation_end_year": int(cfg["end_year"]),
        "time_convention": "Annual timestamps mark the start of the year. Monthly timestamps follow native TEM output.",
        "history": f"{datetime.datetime.now(datetime.timezone.utc):%Y-%m-%dT%H:%M:%SZ} created by src/driver.py",
        **ds.attrs,
    }
    encoding = {"time": time_encoding}
    for name in ds.data_vars:
        encoding[name] = {"zlib": True, "complevel": 4}
        if name == "time_bnds":
            encoding[name].update(time_encoding)
        elif name in integers:
            encoding[name].update(dtype="int32", _FillValue=-1)
        elif ds[name].dtype.kind == "f" and name not in {
            "time_bnds",
            "carea",
            "subarea",
            "area",
        }:
            encoding[name]["dtype"] = "float32"
    for name in ds.coords.keys() & {"lat", "lon"}:
        encoding[name] = {"dtype": "float32"}
    ds.to_netcdf(path, encoding=encoding)
    log.info("Wrote %s", path)
