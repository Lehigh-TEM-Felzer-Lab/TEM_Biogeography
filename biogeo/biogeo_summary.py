# biogeo_summary.py
"""Summary statistics of the biogeography result by PFT and year (replaces xtran)."""

import logging

import numpy as np
import pandas as pd
import xarray as xr
import yaml

import tem_nc

log = logging.getLogger("SUMMARY")


def run(cfg: dict) -> None:
    """Write OUTPUT/summary_annual_<case>_<years>.nc with dims (time, potveg); potveg=region_code holds all PFTs together."""
    ds = tem_nc.open_annual(tem_nc.output_path(cfg, "biogeo"))
    outputs = list(dict.fromkeys([*cfg["temoutvars"], "npp"]))
    df = (
        ds[[*outputs, "potveg", "carea"]]
        .to_dataframe()
        .dropna(subset=["potveg"])
        .reset_index()
    )
    for key in (
        k for k in cfg if k.startswith("summary_") and k != "summary_region_code"
    ):
        value = cfg[key]
        if value is None:
            continue
        column, bound = key.removeprefix("summary_").rsplit("_", 1)
        df = df[df[column] >= value] if bound == "min" else df[df[column] <= value]
    df["potveg"] = df["potveg"].astype(int)
    if df.empty:
        raise ValueError("The summary bounds contain no valid cells or years")
    if cfg["summary_region_code"] in df["potveg"].values:
        raise ValueError("summary_region_code overlaps a selected PFT code")
    regional = df.assign(potveg=cfg["summary_region_code"])
    df = pd.concat([df, regional])

    group = df.groupby(["potveg", "year"])
    summary = xr.Dataset(
        {"ngrid": group.size().to_xarray(), "area": group["carea"].sum().to_xarray()}
    )
    names = yaml.safe_load(cfg["pft_names_file"].read_text())
    tem_nc.describe(summary, "summary")
    stat_names = {
        k: v["long_name"] for k, v in tem_nc.table("summary_statistic").items()
    }
    for var in outputs:
        long_name = ds[var].attrs.get("long_name", var)
        units = ds[var].attrs.get("units", "")
        stats = group[var].agg(["mean", "max", "min", "std"]).to_xarray()
        weighted = (df[var] * df["carea"]).groupby(
            [df["potveg"], df["year"]]
        ).sum() / group["carea"].sum()
        for stat in ("mean", "max", "min", "std"):
            summary[f"{var}_{stat}"] = stats[stat].assign_attrs(
                long_name=f"{long_name}, {stat_names[stat]}", units=units
            )
        summary[f"{var}_area_weighted_mean"] = weighted.to_xarray().assign_attrs(
            long_name=f"{long_name}, {stat_names['area_weighted_mean']}", units=units
        )
        summary[f"{var}_area_integral"] = (
            summary["area"] * stats["mean"] / 1e6
        ).assign_attrs(
            long_name=f"{long_name}, {stat_names['area_integral']}",
            units=f"{units} km2 1e-6".strip(),
            comment="Group arithmetic mean multiplied by total area in square kilometers and divided by 1e6.",
        )
    summary["potveg"].attrs = {
        **tem_nc.table("coordinates")["potveg"],
        "comment": f"{cfg['summary_region_code']} = all PFTs together",
        "flag_values": np.array(
            sorted(c for c in summary.potveg.values if c in names), dtype="int16"
        ),
        "flag_meanings": " ".join(
            names[c].replace(" ", "_")
            for c in sorted(summary.potveg.values)
            if c in names
        ),
    }
    tem_nc.write(
        summary,
        "summary",
        cfg,
        "TEM biogeography summary by PFT and year",
        "Statistics over cells of TEM output variables, grouped by potential vegetation type and year.",
        {"ngrid"},
    )
