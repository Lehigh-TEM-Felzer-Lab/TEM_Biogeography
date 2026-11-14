"""Merge downloaded ERA5 files and an elevation grid into one monthly NetCDF input file for TEM."""

import argparse
from pathlib import Path

import xarray as xr


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--monthly", required=True, help="glob of era5_monthly_*.nc")
    parser.add_argument("--daily", required=True, help="glob of era5_t2m_daily_*.nc")
    parser.add_argument(
        "--elev", type=Path, required=True, help="NetCDF with elevation in m"
    )
    parser.add_argument("--elev-var", default="elev")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    rename = {"valid_time": "time", "latitude": "lat", "longitude": "lon"}
    monthly = xr.open_mfdataset(args.monthly, combine="by_coords")
    daily = xr.open_mfdataset(args.daily, combine="by_coords")
    monthly, daily = (
        d.rename({k: v for k, v in rename.items() if k in d.dims})
        for d in (monthly, daily)
    )
    daily = daily.resample(time="MS").mean()
    monthly["time"] = monthly.time.to_index().to_period("M").to_timestamp()
    elev = xr.open_dataset(args.elev)[args.elev_var].rename(
        {k: v for k, v in rename.items() if k in xr.open_dataset(args.elev).dims}
    )
    ds = xr.merge(
        [
            monthly.drop_vars(["number", "expver"], errors="ignore"),
            daily,
            elev.rename("elev"),
        ],
        join="inner",
    )
    # Monthly-mean ERA5 accumulations are daily means of the accumulation, not monthly totals
    ds["tp"].attrs["units"] = "m day-1"
    ds["ssrd"].attrs["units"] = "J m-2 day-1"
    ds["elev"].attrs["units"] = "m"
    ds.to_netcdf(args.out, encoding={v: {"zlib": True} for v in ds.data_vars})


if __name__ == "__main__":
    main()
