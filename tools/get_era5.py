"""Download ERA5 monthly means and daily 2 m temperature extremes for TEM (needs CDS credentials in ~/.cdsapirc)."""

import argparse
from pathlib import Path

import cdsapi
import xarray as xr

MONTHLY = "reanalysis-era5-single-levels-monthly-means"
DAILY = "derived-era5-single-levels-daily-statistics"
VARIABLES = [
    "2m_temperature",
    "2m_dewpoint_temperature",
    "total_precipitation",
    "surface_solar_radiation_downwards",
    "10m_u_component_of_wind",
    "10m_v_component_of_wind",
]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--years", nargs=2, type=int, required=True, metavar=("START", "END")
    )
    parser.add_argument(
        "--area", nargs=4, type=float, required=True, metavar=("N", "W", "S", "E")
    )
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    years = [str(y) for y in range(args.years[0], args.years[1] + 1)]
    months = [f"{m:02d}" for m in range(1, 13)]
    client = cdsapi.Client()

    target = args.out / f"era5_monthly_{years[0]}_{years[-1]}.nc"
    if not target.exists():
        client.retrieve(
            MONTHLY,
            {
                "product_type": ["monthly_averaged_reanalysis"],
                "variable": VARIABLES,
                "year": years,
                "month": months,
                "time": ["00:00"],
                "area": args.area,
                "data_format": "netcdf",
                "download_format": "unarchived",
            },
            str(target),
        )

    # Daily maximum and minimum of 2 m temperature, one request per year and month
    for statistic, name in (("daily_maximum", "t2m_max"), ("daily_minimum", "t2m_min")):
        for year in years:
            part = args.out / f"era5_t2m_daily_{name}_{year}.nc"
            if part.exists():
                continue
            pieces = []
            for month in months:
                piece = args.out / f"tmp_{name}_{year}{month}.nc"
                client.retrieve(
                    DAILY,
                    {
                        "product_type": "reanalysis",
                        "variable": ["2m_temperature"],
                        "year": year,
                        "month": month,
                        "day": [f"{d:02d}" for d in range(1, 32)],
                        "daily_statistic": statistic,
                        "time_zone": "utc+00:00",
                        "frequency": "1_hourly",
                        "area": args.area,
                    },
                    str(piece),
                )
                pieces.append(piece)
            with xr.open_mfdataset([str(p) for p in pieces], combine="by_coords") as ds:
                var = next(iter(ds.data_vars))
                ds[[var]].rename({var: name}).load().to_netcdf(part)
            for piece in pieces:
                piece.unlink()


if __name__ == "__main__":
    main()
