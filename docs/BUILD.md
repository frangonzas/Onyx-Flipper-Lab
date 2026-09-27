# Build and Install

## Requirements

- Flipper Zero
- USB cable
- Python 3.8 or newer for uFBT bootstrap
- microSD card installed in the Flipper
- uFBT

## Install uFBT

macOS / Linux:

```bash
python3 -m pip install --upgrade ufbt
```

Windows:

```powershell
py -m pip install --upgrade ufbt
```

## Build

Clone the repository and enter it:

```bash
git clone https://github.com/frangonzas/Onyx-Flipper-Lab.git
cd Onyx-Flipper-Lab
ufbt
```

A successful build places the FAP in `dist/`.

## Run directly on a connected Flipper

```bash
ufbt launch
```

This compiles the project, transfers the FAP over USB and launches it.

## SDK compatibility

uFBT downloads an SDK for the selected firmware channel.

If the FAP reports an SDK/API mismatch, update the local SDK or build against the firmware family installed on your device.

## Manual transfer

A built FAP can also be copied to the Flipper microSD card using qFlipper's file manager.

Place it under an appropriate directory inside:

```text
/ext/apps/
```

The exact menu category is defined by `fap_category` in `application.fam`.

## Logs

The application writes basic lifecycle messages through Furi logging.

With the device connected:

```bash
ufbt cli
```

Then enable/view the desired log level from the Flipper CLI.
