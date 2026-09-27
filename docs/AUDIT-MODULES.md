# Audit Modules

## GPIO Auditor

Purpose: inspect analog voltage on ADC-capable external pins without driving outputs.

Capabilities:

- enumerates non-debug ADC-capable GPIO records exposed by the firmware;
- configures the selected pin as analog input;
- reads the 12-bit ADC value;
- converts it to millivolts using the firmware calibration API;
- tracks session sample count;
- tracks session-wide minimum and maximum measured voltage.

Risk boundary: the module never configures a pin as output and never writes a digital level.

## Sub-GHz RSSI Monitor

Purpose: passive RF presence assessment.

Capabilities:

- uses the internal CC1101;
- loads the official asynchronous OOK RX preset;
- switches between several common hardware-supported frequencies;
- reports current RSSI, peak RSSI and LQI;
- accumulates session RSSI min/max;
- lets the operator capture the current RSSI as a baseline;
- displays live delta from baseline;
- labels a change when delta exceeds ±12 dB.

The threshold is a simple field indicator, not an RF classification engine.

The module contains no transmit path, encoder, replay or key handling.

## Bluetooth Audit

Purpose: inspect local Bluetooth controller posture using public HAL APIs.

Capabilities:

- controller alive state;
- active advertising/connection state;
- GATT/GAP capability;
- test-mode capability flag;
- link RSSI when Bluetooth is active.

The public FAP API does not currently provide a stable generic BLE advertisement scanner, so the module deliberately avoids private/internal BLE glue APIs.

## Infrared Inspector

Purpose: inspect IR traffic from equipment you are authorized to test.

Capabilities:

- receives IR signals;
- enables firmware signal decoding;
- identifies supported protocols;
- displays address, command and repeat state in volatile memory;
- reports raw timing count for undecoded signals;
- increments a session-wide signal counter.

No IR transmit function is used.

## NFC Detector

Purpose: inventory the protocol family of a presented NFC object.

Capabilities:

- uses the official greedy `NfcScanner`;
- identifies one or more supported protocols;
- reports up to four detected protocol identifiers;
- maintains a session-wide detection-event counter;
- keeps detected data in RAM.

It does not read application sectors, authenticate, emulate, write or modify cards.

The exported field report stores only the count of NFC detection events, not UIDs.

## LF RFID Reader

Purpose: identify LF RFID credentials that the operator owns or is authorized to inspect.

Capabilities:

- automatic ASK/PSK read mode;
- protocol-name identification;
- bounded volatile preview of up to 8 bytes;
- session-wide detection-event counter.

The module never calls LF-RFID write or emulation APIs.

The exported field report stores only the number of detection events, not payload bytes.

## Session Report

Purpose: provide one privacy-conscious summary of the current field session.

The screen displays:

- GPIO samples and min/max voltage;
- Sub-GHz samples and RSSI range;
- IR signal count;
- NFC detection-event count;
- LF RFID detection-event count;
- last export status.

Pressing **OK** writes `last_audit_report.txt` into the app data directory.

## Randomness utilities

Password and random-HEX tools use the firmware hardware RNG.

Password character selection uses rejection sampling to avoid simple modulo bias.
