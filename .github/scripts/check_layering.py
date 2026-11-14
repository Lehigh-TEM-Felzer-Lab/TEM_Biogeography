#!/usr/bin/env python3
"""Checks the include rule of src/ given in .github/layering.yaml.

`layers` lists, for each directory of src/, the layers it may include from. `forbid_system` lists system headers a layer must not
include. `exceptions` are the includes that break the rule today; a new violation fails, and so does an exception that no longer
occurs (remove it), so the list can only shrink. `--update` rewrites `exceptions` from the sources.
"""
import pathlib
import re
import sys

import yaml

root = pathlib.Path(__file__).resolve().parents[2]
cfg_file = root / ".github" / "layering.yaml"
cfg = yaml.safe_load(cfg_file.read_text())
files = {d: {p.name for p in (root / "src" / d).iterdir()} for d in cfg["layers"]}
include = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)

found: dict[str, set[str]] = {}
for layer, rule in cfg["layers"].items():
    banned = [d for d in files if d != layer and d not in rule["may_include"]]
    for p in sorted((root / "src" / layer).iterdir()):
        for h in include.findall(p.read_text(errors="replace")):
            if any(h in files[b] for b in banned) or h in rule.get("forbid_system", []):
                found.setdefault(f"{layer}/{p.name}", set()).add(h)

if "--update" in sys.argv:
    text = cfg_file.read_text()
    head = text[: text.index("exceptions:")]
    cfg_file.write_text(head + yaml.safe_dump({"exceptions": {k: sorted(v) for k, v in sorted(found.items())}}, default_flow_style=False))
    sys.exit(0)

allowed = {k: set(v) for k, v in (cfg.get("exceptions") or {}).items()}
pairs = lambda d: {(k, h) for k, v in d.items() for h in v}
new, gone = sorted(pairs(found) - pairs(allowed)), sorted(pairs(allowed) - pairs(found))
for k, h in new:
    print(f"new violation: {k}: {h}")
for k, h in gone:
    print(f"fixed, remove from exceptions in .github/layering.yaml: {k}: {h}")
print(f"{len(pairs(found))} include(s) cross the layers, {len(pairs(allowed))} allowed by the exceptions")
sys.exit(bool(new or gone))
