from pathlib import Path

import pandas as pd
import xarray as xr


def load_nc(path: str):

    ds = xr.open_dataset(path)
    print("Dimensions:", list(ds.dims))
    print("Data variables:", list(ds.data_vars))
    return ds


def mean_wind(df: pd.DataFrame, varname: str):
    mean = df.groupby(["lon", "lat"], as_index=False).agg(
        {
            "total": "mean",
            "max": "mean",
            "avg": "mean",
            "min": "mean",
            "jan": "mean",
            "feb": "mean",
            "mar": "mean",
            "apr": "mean",
            "may": "mean",
            "jun": "mean",
            "jul": "mean",
            "aug": "mean",
            "sep": "mean",
            "oct": "mean",
            "nov": "mean",
            "dec": "mean",
        }
    )
    mean["year"] = -999
    mean["varname"] = f" {varname} ".upper()

    return mean


def extract(data: xr.Dataset, var: str, var_out_name: str):

    df = pd.DataFrame()

    monthly = data.resample(time="1ME").mean()

    annual = monthly.groupby("time.year")
    sum_vals = annual.sum("time")
    mean_vals = annual.mean("time")
    max_vals = annual.max("time")
    min_vals = annual.min("time")

    months = {
        m: monthly.sel(time=monthly["time.month"] == m)
        .groupby("time.year")
        .mean("time")
        for m in range(1, 13)
    }

    for year in annual.groups.keys():
        print(f"  Processing year: {year}")
        # Select per-year arrays
        sum_y = sum_vals.sel(year=year).to_dataframe().reset_index()
        avg_y = mean_vals.sel(year=year).to_dataframe().reset_index()
        max_y = max_vals.sel(year=year).to_dataframe().reset_index()
        min_y = min_vals.sel(year=year).to_dataframe().reset_index()

        # Start with lon/lat
        tmp = sum_y[["lon", "lat"]].copy()
        tmp["varname"] = f" {var_out_name} ".upper()
        tmp["year"] = year
        tmp["total"] = sum_y[var].values
        tmp["max"] = max_y[var].values
        tmp["avg"] = avg_y[var].values
        tmp["min"] = min_y[var].values

        # Add months
        for m, name in zip(
            range(1, 13),
            [
                "jan",
                "feb",
                "mar",
                "apr",
                "may",
                "jun",
                "jul",
                "aug",
                "sep",
                "oct",
                "nov",
                "dec",
            ],
        ):
            if year in months[m]["year"]:
                print(f"    Processing month: {m}")
                tmp[name] = (
                    months[m].sel(year=year).to_dataframe().reset_index()[var].values
                )
        df = pd.concat([df, tmp], ignore_index=True)

    return df


def preprocess(
    input_nc_dict,
    grids=None,
    start_year=None,
    end_year=None,
    get_mean_wind=False,
):

    data_list = ["dtr", "nirr", "prec", "tair", "vpr", "was"]
    var_out_names = ["DTAIRRNG", "NIRR", "PREC", "TAIR", "VPRPRESS", "WIND"]

    for var, varname in zip(data_list, var_out_names):

        if var not in input_nc_dict:
            continue

        print(f"Processing variable: {var}")

        if var == "dtr":
            input_nc_dict1 = input_nc_dict.get("tmin", None)
            input_nc_dict2 = input_nc_dict.get("tmax", None)

            if input_nc_dict1 is None or input_nc_dict2 is None:
                raise ValueError(
                    """Both 'tmin' and 'tmax' input files must be provided to calculate 'dtr'. Example:
                'dtr': {'tmin':"path_to_tmin_file", 'tmax':"path_to_tmax_file", 'varname':"variable_name"}"""
                )

            tmin = input_nc_dict1.get("path", None)
            tmax = input_nc_dict2.get("path", None)
            varname = input_nc_dict1.get("varname", None)

            if tmin is None or tmax is None or varname is None:
                raise ValueError(
                    "dtr input needs 'tmin' and 'tmax' paths and 'varname'"
                )

            ds_tmin = load_nc(tmin)
            ds_tmax = load_nc(tmax)

            dtr = ds_tmax["air_temperature"] - ds_tmin["air_temperature"]
            dtr = dtr.rename("dtr")

            df = extract(dtr, var, varname)

        elif var == "tair":
            input_nc_dict1 = input_nc_dict.get("tmin", None)
            input_nc_dict2 = input_nc_dict.get("tmax", None)

            if input_nc_dict1 is None or input_nc_dict2 is None:
                raise ValueError(
                    "Both 'tmin' and 'tmax' input files must be provided to calculate 'tair'. Example: 'tair': {'tmin':path_to_tmin_file, 'tmax':path_to_tmax_file, 'varname':variable_name}"
                )

            tmin = input_nc_dict1.get("path", None)
            tmax = input_nc_dict2.get("path", None)
            varname = input_nc_dict1.get("varname", None)
            if tmin is None or tmax is None or varname is None:
                raise ValueError(
                    "tair input needs 'tmin' and 'tmax' paths and 'varname'"
                )

            ds_tmin = load_nc(tmin)
            ds_tmax = load_nc(tmax)

            tair = (
                (ds_tmax["air_temperature"] + ds_tmin["air_temperature"]) / 2
            ) - 273.15

            tair = tair.rename("tair")
            tair = tair.to_dataset()
            df = extract(tair, var, varname)

        elif var == "vpr":
            specific_humidity = input_nc_dict.get(var, None)
            if specific_humidity is None:
                raise ValueError(
                    "'vpr' input file must be provided Example: 'vpr': {'path': 'path_to_vpr_file', 'varname': 'variable_name'}"
                )

            ds_vpr = xr.open_dataset(specific_humidity)
            vpr = ds_vpr[varname] * 1010 / (0.622 + (1 - 0.622) * ds_vpr[varname])
            vpr = vpr.rename("vpr")
            vpr = vpr.to_dataset()
            df = extract(vpr, var, varname)
            # (q * p) / (epsilon + (1 - epsilon) * q)

        else:
            input_nc_dict = input_nc_dict.get(var, None)
            if input_nc_dict is None:
                raise ValueError(
                    f'"{var}" input file must be provided Example: "{var}": {{"path": "path_to_{var}_file", "varname": "variable_name"}}'
                )

            path = input_nc_dict["path"]
            varname = input_nc_dict["varname"]
            ds = load_nc(path)
            df = extract(ds, var, varname)

            if var == "was" and get_mean_wind:
                df = mean_wind(df, varname)

        if start_year:
            df = df[df["year"] >= start_year]
        if end_year:
            df = df[df["year"] <= end_year]

        if grids:
            grid_df = pd.read_csv(
                grids,
            )
            df = pd.merge(grid_df, df, on=["lon", "lat"], how="left")

        else:
            df["region"] = "GLOBAL"
            df["area"] = -9999

        for col in df.columns:
            if col in ["varname", "region"]:
                df[col] = df[col].astype(str)
            elif col in ["year", "area"]:
                df[col] = df[col].astype(int)
            else:
                df[col] = df[col].astype(float).round(2)

        df = df[
            [
                "lon",
                "lat",
                "varname",
                "area",
                "year",
                "total",
                "max",
                "avg",
                "min",
                "jan",
                "feb",
                "mar",
                "apr",
                "may",
                "jun",
                "jul",
                "aug",
                "sep",
                "oct",
                "nov",
                "dec",
                "region",
            ]
        ]

        start_year = int(df["year"].min())
        end_year = int(df["year"].max())

        mid = f"_{start_year}_{end_year}"
        if start_year == end_year:
            mid = ""

        out_csv = Path(f"./output/{var}{mid}.csv")
        out_csv.parent.mkdir(parents=True, exist_ok=True)
        df.to_csv(out_csv, index=False, header=True)


if __name__ == "__main__":
    grids = "grids.csv"

    # Example
    data_dicts = [
        {
            "TAIR": {
                "path": "2m_temperature.nc",
                "varname": "t2m",
            },
        },
    ]

    for data_dict in data_dicts:
        preprocess(data_dict, grids=grids, get_mean_wind=True)
