"""Derive annual cohort and monthly moisture diagnostics from TEM NetCDF output."""

from pathlib import Path

import xarray as xr

import tem_nc

META = ("standage", "potveg", "currentveg", "subtype", "cmnt", "carea", "subarea")
INTEGERS = set(META) - {"carea", "subarea"}


def annual_output(ds: xr.Dataset, names: list[str]) -> xr.Dataset:
    """Sum monthly fluxes, average states, and retain the final cohort metadata."""
    annual = {}
    for name in names:
        da = ds[name].astype("float64")
        units = da.attrs.get("units", "")
        annual[name] = da.groupby("time.year").mean("time", keep_attrs=True)
        if units.endswith("month-1"):
            annual[f"{name}_avg"] = annual[name]
            annual[name] = da.groupby("time.year").sum(
                "time", min_count=12, keep_attrs=True
            )
            annual[name].attrs["units"] = units.removesuffix("month-1") + "yr-1"
        elif not units:
            annual[f"{name}_avg"] = annual[name]
            annual[name] = da.groupby("time.year").sum(
                "time", min_count=12, keep_attrs=True
            )
    for name in META:
        annual[name] = ds[name].groupby("time.year").last(skipna=True)
    return xr.Dataset(annual)


def run(cfg: dict) -> None:
    """Write the cohort and moisture products used by the PFT selector."""
    with xr.open_dataset(Path(cfg["output_file"]), engine="netcdf4") as native:
        ds = native.sel(time=slice(str(cfg["start_year"]), str(cfg["end_year"])))
        years = ds.time.dt.year.values
        months = ds.time.dt.month.values
        expected = [
            (year, month)
            for year in range(cfg["start_year"], cfg["end_year"] + 1)
            for month in range(1, 13)
        ]
        if list(zip(years, months, strict=True)) != expected:
            raise ValueError(
                "Native TEM output must contain each requested month exactly once"
            )
        names = list(dict.fromkeys([*cfg["temoutvars"], "npp"]))
        required = {*names, *META, "actevap", "potevap"}
        if missing := required - ds.data_vars.keys():
            raise ValueError(f"TEM output is missing variables {sorted(missing)}")
        tem_nc.write(
            annual_output(ds, names),
            "tem_annual",
            cfg,
            "TEM annual output by cohort",
            "Annual flux sums and mean monthly rates, annual mean states, and cohort metadata.",
            INTEGERS,
        )
        monthly = ds[[*names, "time_bnds"]]
        tem_nc.write(
            monthly,
            "tem_monthly",
            cfg,
            "TEM monthly output by cohort",
            "Native TEM monthly diagnostics.",
        )
        pet = ds.potevap.where(ds.potevap > 0)
        ratio = ds.actevap / pet
        moisture = ds[["potveg", "subtype", "actevap", "potevap", "time_bnds"]].rename(
            actevap="aet", potevap="pet"
        )
        moisture["aet_pet"] = ratio
        moisture["dryness"] = 1 - ratio
        tem_nc.write(
            tem_nc.describe(moisture, "mmdi"),
            "mmdi",
            cfg,
            "TEM monthly moisture stress by cohort",
            "Monthly AET and PET. Ratios are missing where PET is zero or negative.",
            {"potveg", "subtype"},
        )
        if cfg["veg_fire"]:
            tem_nc.write(
                ds[["areaburn", "potveg", "subtype", "time_bnds"]],
                "fire",
                cfg,
                "TEM monthly fire diagnostics by cohort",
                "Fraction of each cohort burned by wildfire (GDSTEM-FIRE).",
                {"areaburn", "potveg", "subtype"},
            )
