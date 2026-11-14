#!/usr/bin/env python3
"""Rebuild biogeography products from the staged climate and native TEM output."""

import logging
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

import biogeo_select
import biogeo_summary

import tem_config
import tem_output_nc


def run(cfg: dict) -> None:
    """Select and summarize PFTs, and optionally map the persisting types."""
    biogeo_select.run(cfg)
    biogeo_summary.run(cfg)
    if cfg["plot_biogeography"]:
        import biogeo_plot

        biogeo_plot.run(cfg)


def main() -> None:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)s %(message)s")
    cfg = tem_config.load()
    tem_output_nc.run(cfg)
    run(cfg)


if __name__ == "__main__":
    main()
