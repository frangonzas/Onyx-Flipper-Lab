# Report Format

## File

The latest exported field report is written to:

```text
/ext/apps_data/onyx_flipper_lab/last_audit_report.txt
```

Each export intentionally replaces the previous report.

This design minimizes uncontrolled accumulation of observed field data.

## Example

```text
ONYX FLIPPER LAB - FIELD AUDIT REPORT
Version: 3.0
Operator: Fran Gonzas
Scope: defensive / authorized / read-only radio

Session start: 2026-09-27 12:00:00
Report time:   2026-09-27 12:15:00

[GPIO]
samples=42
min_mv=0
max_mv=3294

[SUBGHZ_RX]
samples=105
min_rssi_dbm=-103.4
max_rssi_dbm=-44.7
tx_performed=false

[SIGNAL_INVENTORY]
ir_signals=4
nfc_detection_events=2
lf_rfid_detection_events=1

[PRIVACY]
credential_payloads_saved=false
ir_commands_saved=false
nfc_uid_saved=false
lf_rfid_bytes_saved=false

END OF REPORT
```

## Privacy model

The report is a measurement summary, not a credential database.

It intentionally excludes:

- NFC UID values;
- NFC tag application data;
- LF RFID credential bytes;
- IR addresses and commands;
- passwords generated in-app;
- raw or replayable Sub-GHz captures.

## Interpretation

RSSI is environmental and highly dependent on distance, antenna orientation, interference and receiver configuration.

The Sub-GHz baseline delta is a simple comparative indicator and must not be interpreted as proof of device identity or malicious activity.
