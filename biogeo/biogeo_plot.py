# biogeo_plot.py
"""Map the persisting potential vegetation of each period from biogeo.nc."""

import logging

import cartopy.crs as ccrs
import cartopy.feature as cfeature
import matplotlib.pyplot as plt
import numpy as np
import xarray as xr

import tem_nc

log = logging.getLogger("PLOT")


def run(cfg: dict) -> None:
    """Write OUTPUT/potveg_periods_<case>_<years>.png with one panel per biogeography period."""
    ds = xr.open_dataset(tem_nc.output_path(cfg, "biogeo"))
    periods = [v for v in ds.data_vars if v.startswith("potveg_")]
    flags = ds[periods[0]].attrs
    codes = np.asarray(flags["flag_values"])
    labels = flags["flag_meanings"].split()
    cmap = plt.get_cmap("tab20", len(codes))
    fig, axes = plt.subplots(
        1,
        len(periods),
        figsize=(5 * len(periods), 4),
        subplot_kw={"projection": ccrs.PlateCarree()},
    )
    for ax, name in zip(np.atleast_1d(axes), periods, strict=True):
        index = xr.apply_ufunc(
            lambda x: np.searchsorted(codes, x).astype(float), ds[name]
        ).where(ds[name].notnull())
        mesh = index.plot(
            ax=ax, cmap=cmap, vmin=-0.5, vmax=len(codes) - 0.5, add_colorbar=False
        )
        ax.add_feature(cfeature.STATES, linewidth=0.3)
        ax.set_title(name.removeprefix("potveg_"))
    cbar = fig.colorbar(mesh, ax=axes, ticks=range(len(codes)), shrink=0.8)
    cbar.ax.set_yticklabels(labels)
    path = tem_nc.output_path(cfg, "potveg_periods", ".png")
    fig.savefig(path, dpi=200, bbox_inches="tight")
    log.info("Wrote %s", path)
