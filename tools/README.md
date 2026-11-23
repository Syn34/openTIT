# Analysis tools

These scripts work on your own decrypted `EBOOT.elf` and produce local reference data.
None of their output is committed except the function/import lists in `docs/`.

| Script | Purpose |
|---|---|
| `find_functions.py EBOOT.elf out.tsv` | function entry points from `.opd` descriptors + `bl` targets |
| `resolve_imports.py imports.tsv 'names/*' out.tsv` | names PRX import stubs by NID |
| `rtti_scan.py EBOOT.elf out.tsv` | class names and vtables from Itanium RTTI (mostly Havok) |
| `build_index.py ...` | per-function callees/strings index from reference listings |
| `progress.py` | counts `@func` tags and updates README.md |
