# Changelog

## 3.0.0 — Field Audit Terminal · FINAL

### Final validation

- compiled successfully with the official Flipper release SDK;
- installed and accepted as tested on physical Flipper Zero hardware;
- project status set to FINAL / MAINTENANCE ONLY;
- no further feature development planned unless the project is reopened.

### Added

- session-wide GPIO sample count;
- GPIO minimum/maximum voltage tracking;
- session-wide Sub-GHz RSSI min/max tracking;
- operator-defined Sub-GHz RSSI baseline;
- live RSSI delta and simple ±12 dB change indicator;
- session-wide IR signal counter;
- session-wide NFC detection-event counter;
- session-wide LF RFID detection-event counter;
- Session Report screen;
- privacy-safe microSD report export;
- report format documentation.

### Changed

- application version moved to 3.0;
- stack allocation increased to support expanded field-session workflow;
- documentation now describes persistence and privacy explicitly.

### Security

- exported report excludes credential payloads, NFC UIDs and IR command values;
- radio and credential modules remain read-only / receive-only.

## 2.0.0 — Hardware Audit Suite

- GPIO ADC auditor;
- Sub-GHz RSSI/LQI RX monitor;
- Bluetooth local posture view;
- IR receive inspector;
- NFC protocol detector;
- LF RFID read inspector.

## 1.0.0 — Initial Release

- hardware-RNG password generator;
- random HEX generator;
- security guidance;
- basic Flipper FAP structure and CI.
