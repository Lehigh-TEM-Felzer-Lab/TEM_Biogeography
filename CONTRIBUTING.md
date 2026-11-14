# Contributing

## Source layout

```
src/physics/   the science and the records it uses (vegetation, soil, microbes, evapotranspiration, fire, land use,
               cell and cohort classes, constants, time step)
src/io/        files and processes: YAML, NetCDF, restart state, input checks, log, diagnostics transport, MPI layer
src/driver/    xtem_gds.cpp, the main program, which wires physics and io together
```

- `io` and `driver` may include from `physics`. `physics` includes nothing from `io` or `driver`. `io` includes nothing from `driver`.
- `python3 .github/scripts/check_layering.py` checks this. The includes that break the rule today are the `exceptions` of `.github/layering.yaml`
  (YAML readers in the `getecd` methods, NetCDF reads in the record classes, the restart state in `Telm45`). A new violation
  fails, and so does an entry that is gone: delete it with `python3 .github/scripts/check_layering.py --update`. The list may only shrink.
- All three directories are on the include path, so `#include "tem_yaml.h"` needs no directory. File names must stay unique across them.
- A new file goes where its role puts it: if it reads a file, calls NetCDF, YAML or MPI, or writes the log, it is `io`; if it
  computes the model, it is `physics`. Do not reformat or rename files in `physics` for style: the science is carried over from GDSTEM by diff,
  and every changed line becomes a possible conflict.

## Model sources are a copy of XTEM

`src/physics`, `src/io` and `src/driver` are the same files as in [XTEM](https://github.com/Lehigh-TEM-Felzer-Lab/XTEM) (the Python case
builder is `src/*.py`; `src/driver.py` is the case builder, `src/driver/` the model's main program). Change the model in XTEM first,
then copy the three directories here. `.github/workflows/core-parity.yml` fails on a pull request, and weekly, when they differ.
Science changes come to XTEM from [GDSTEM](https://github.com/Lehigh-TEM-Felzer-Lab/GDSTEM) (see CONTRIBUTING.md of XTEM); they
reach this repository through XTEM, not directly.

## Checks

`make && make test` builds and runs the unit tests. `ci.yml` also runs the layering and structure checks. The reference run of
the model (`examples/test`, `tests/golden`) is in XTEM.
