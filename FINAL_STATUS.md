# Final Project Status

## Onyx Flipper Lab v3.0.0

**Status:** TESTED · FINALIZED · MAINTENANCE ONLY  
**Owner:** Fran Gonzas  
**Platform:** Flipper Zero  
**Application type:** External FAP  
**Language:** C  
**Build system:** uFBT / official Flipper release SDK

## Validation

The final project state has:

- compiled successfully in GitHub Actions against the official release SDK;
- produced a valid external `.fap`;
- been installed on physical Flipper Zero hardware;
- been accepted as the finished project version by the project owner.

## Final scope

The final version includes:

- GPIO ADC auditing with session metrics;
- Sub-GHz receive-only RSSI/LQI monitoring and baseline delta;
- Bluetooth local posture/status auditing;
- infrared receive inspection;
- NFC protocol detection;
- LF RFID read inspection;
- hardware RNG utilities;
- field-session metrics;
- privacy-safe report export to microSD.

## Security boundary

The project remains intentionally defensive:

- no Sub-GHz transmission or replay;
- no IR transmission;
- no NFC emulation or write;
- no LF RFID write or emulation;
- no brute force;
- no jamming;
- no hidden persistence;
- no exported credential payloads.

## Maintenance policy

The codebase is considered complete.

Future changes should be limited to:

- compatibility fixes for future Flipper firmware/API changes;
- build-system maintenance;
- security corrections;
- documentation corrections.

New feature work should only resume if the project is explicitly reopened.

---

© 2026 Fran Gonzas
