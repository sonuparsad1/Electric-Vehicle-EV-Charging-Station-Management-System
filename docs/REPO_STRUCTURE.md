# Repository Structure

## Active application (production)
- `app/src/` -> WinAPI C source files
- `app/include/` -> public headers

## Build and packaging entry points
- `Build-App-OneClick.bat` -> one-click Windows build + run
- `Package-App-OneClick.bat` -> one-click Windows portable ZIP package
- `scripts/OneClick-Build-Run.bat` -> internal build workflow
- `scripts/Create-Portable-Package.bat` -> internal packaging workflow

## Compatibility layer
- `main.c`, `billing.c`, `billing.h` -> root shims pointing to canonical `app/` files

## Archived legacy code
- `legacy/root-c/` -> old flat C files
- `legacy/src/` and `legacy/include/` -> old split source/header layout

## Documentation
- `README.md` -> setup and usage guide
- `docs/SAMPLE_OUTPUT.txt` -> legacy sample output reference
