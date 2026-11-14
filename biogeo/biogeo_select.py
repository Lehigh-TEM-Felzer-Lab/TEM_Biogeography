# biogeo_select.py
"""Biogeography module: select the most productive climatically possible PFT in each cell and year."""

import logging
from pathlib import Path

import numpy as np
import pandas as pd
import xarray as xr
import yaml

import tem_nc

log = logging.getLogger("BIOGEO")
KEYS = ["lon", "lat", "year"]


def annual_climate(path: Path, base_temperature: float) -> xr.Dataset:
    """Compute climate limits using the same no-leap months as the model."""
    with xr.open_dataset(path) as ds:
        tair, prec = ds.tair, ds.prec
        days = ds.time.dt.days_in_month
        return xr.Dataset(
            {
                "gdd": ((tair - base_temperature).clip(min=0) * days)
                .groupby("time.year")
                .sum(min_count=12),
                "t_min": tair.groupby("time.year").min(),
                "t_max": tair.groupby("time.year").max(),
                "annual_prec": prec.groupby("time.year").sum(min_count=12),
            }
        ).load()


def summer_moisture(path: Path, months: list[int]) -> pd.DataFrame:
    """Mean AET/PET per PFT and year over summer records with positive PET."""
    with xr.open_dataset(path) as ds:
        summer = ds[["potveg", "subtype", "aet_pet"]].sel(
            time=ds.time.dt.month.isin(months)
        )
        frame = summer.assign_coords(year=summer.time.dt.year).to_dataframe()
    return (
        frame.dropna(subset=["potveg"])
        .reset_index()
        .groupby([*KEYS, "potveg", "subtype"], as_index=False)["aet_pet"]
        .mean()
    )


def possible_mask(df: pd.DataFrame, use_max_gdd: bool) -> pd.Series:
    ok = (
        (df["gdd"] >= df["min_gdd"])
        & (df["annual_prec"] >= df["min_annual_prec"])
        & (df["t_min"] >= df["tc"])
        & (df["t_max"] <= df["tw"])
        & (df["aet_pet"] >= df["min_aet_pet"])
    )
    return ok & (df["gdd"] <= df["max_gdd"]) if use_max_gdd else ok


def select_winners(df: pd.DataFrame) -> pd.DataFrame:
    """One row per cell and year: highest NPP among possible PFTs, else highest NPP overall."""
    order = df.sort_values(
        [*KEYS, "possible", "npp", "cohort"],
        ascending=[True, True, True, False, False, True],
    )
    return order.drop_duplicates(KEYS, keep="first")


def periods(cfg: dict) -> list[dict]:
    """Inclusive year ranges of the persisting-vegetation periods from period_names and period_ends."""
    first, last = cfg["start_year"], cfg["end_year"]
    starts = [first, *[end + 1 for end in cfg["period_ends"]]]
    ends = [*cfg["period_ends"], last]
    return [
        {"name": name, "start": max(start, first), "end": min(end, last)}
        for name, start, end in zip(cfg["period_names"], starts, ends, strict=True)
        if max(start, first) <= min(end, last)
    ]


def persisting(winners: pd.DataFrame, periods: list[dict]) -> dict[str, pd.Series]:
    """Most frequent winning PFT per cell and period; ties go to the larger summed NPP, then the lower code."""
    out = {}
    for period in periods:
        start, end = period["start"], period["end"]
        sel = winners[winners["year"].between(start, end)]
        count = sel.groupby(["lon", "lat", "potveg"], as_index=False).agg(
            years=("year", "size"), npp=("npp", "sum")
        )
        best = count.sort_values(
            ["lon", "lat", "years", "npp", "potveg"],
            ascending=[True, True, False, False, True],
        )
        out[period["name"]] = best.drop_duplicates(["lon", "lat"]).set_index(
            ["lat", "lon"]
        )["potveg"]
        log.info("Persisting PFT %s: %d-%d", period["name"], start, end)
    return out


def run(cfg: dict) -> None:
    """Write OUTPUT/biogeo_annual_<case>_<years>.nc from the TEM annual and moisture files and staged NetCDF climate."""
    tem = tem_nc.open_annual(tem_nc.output_path(cfg, "tem_annual"))
    outputs = list(dict.fromkeys([*cfg["temoutvars"], "npp"]))

    limits = pd.DataFrame(yaml.safe_load(cfg["pft_limits_file"].read_text())).drop(
        columns="description", errors="ignore"
    )
    names = yaml.safe_load(cfg["pft_names_file"].read_text())
    climate = (
        annual_climate(
            cfg["case_dir"] / "INPUT" / "climate.nc", cfg["base_temperature"]
        )
        .to_dataframe()
        .reset_index()
    )
    moisture = summer_moisture(tem_nc.output_path(cfg, "mmdi"), cfg["summer_months"])

    df = (
        tem[
            [
                *outputs,
                *["standage", "potveg", "currentveg", "subtype", "cmnt", "carea"],
            ]
        ]
        .to_dataframe()
        .reset_index()
    )
    df = df.dropna(subset=["npp", "potveg"])
    if df.empty:
        raise ValueError(
            "No cohorts produced valid NPP. Check the TEM log for initialization failures."
        )
    df = df.merge(
        limits, validate="many_to_one", on=["potveg", "subtype"], how="left"
    ).merge(climate, on=KEYS, how="left")
    df = df.merge(
        moisture, on=[*KEYS, "potveg", "subtype"], how="left", validate="many_to_one"
    )
    df["possible"] = possible_mask(df, cfg["use_max_gdd"])

    winners = select_winners(df)
    winners["subarea"] = winners["carea"]  # the selected PFT occupies the whole cell
    any_possible = df.groupby(KEYS)["possible"].any().rename("any_possible")
    winners = winners.merge(any_possible.reset_index(), on=KEYS)

    ds = (
        winners.set_index(["year", "lat", "lon"])
        .drop(columns=["cohort", *limits.columns[2:]])
        .to_xarray()
    )
    for var in ("possible", "any_possible"):
        ds[var] = ds[var].astype(float)
    for var in outputs:
        ds[var].attrs = tem[var].attrs

    for name, series in persisting(winners, periods(cfg)).items():
        ds[f"potveg_{name}"] = series.to_xarray()

    present = [v for v in ds.data_vars if v.startswith("potveg")]
    codes = sorted(
        {
            int(c)
            for v in present
            for c in np.unique(ds[v].values[~np.isnan(ds[v].values)])
        }
    )
    flags = {
        "flag_values": np.array(codes, dtype="int16"),
        "flag_meanings": " ".join(
            names.get(c, f"PFT_{c}").replace(" ", "_") for c in codes
        ),
    }
    for v in present:
        ds[v].attrs.update(flags)
    flags01 = np.array([0, 1], dtype="int16")
    ds["possible"].attrs = {
        "flag_values": flags01,
        "flag_meanings": "not_possible possible",
    }
    ds["any_possible"].attrs = {
        "flag_values": flags01,
        "flag_meanings": "none_possible some_possible",
    }
    method = (
        "Highest-NPP PFT among those whose climate limits (GDD, coldest and warmest "
        "month temperature, annual precipitation, summer AET/PET) are met; "
        "highest-NPP PFT overall if none is possible."
    )
    period_attrs = tem_nc.table("biogeo")["potveg_period"]
    for period in periods(cfg):
        ds[f"potveg_{period['name']}"].attrs.update(
            long_name=f"{period_attrs['long_name']}: {period['name']}",
            units=period_attrs["units"],
            period_start_year=period["start"],
            period_end_year=period["end"],
            comment="PFT selected in most years of the period; ties go to larger summed NPP, then lower code.",
        )
    tem_nc.describe(tem_nc.describe(ds, "biogeo"), "metadata")
    ds.attrs["method"] = method

    integers = {
        "standage",
        "potveg",
        "possible",
        "any_possible",
        "currentveg",
        "subtype",
        "cmnt",
        *[v for v in ds.data_vars if v.startswith("potveg_")],
    }
    tem_nc.write(
        ds,
        "biogeo",
        cfg,
        "TEM biogeography: potential vegetation by year and period",
        "Potential vegetation type from TEM NPP constrained by climatic limits of each PFT.",
        integers,
    )
