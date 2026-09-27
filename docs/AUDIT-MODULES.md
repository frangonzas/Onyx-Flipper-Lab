# Audit Modules

## GPIO Auditor

Purpose: inspect analog voltage on ADC-capable external pins without driving outputs.

Capabilities:

- enumerates non-debug ADC-capable GPIO records exposed by the firmware;
- configures the selected pin as analog input;
- reads the 12-bit ADC value;
- converts it to millivolts using the firmware calibration API;
- allows manual pin selection.

Risk boundary: the module never configures a pin as output and never writes a digital level.

## Sub-GHz RSSI Monitor

Purpose: passive RF presence assessment.

Capabilities:

- uses the internal CC1101;
- loads the official OOK async RX preset;
- switches between several common hardware-supported frequencies;
- reports current RSSI, peak RSSI and LQI;
- remains in receive mode.

The module contains no transmit path, encoder, replay or key handling.

## Bluetooth Audit

Purpose: inspect local Bluetooth controller posture using public HAL APIs.

Capabilities:

- controller alive state;
- active advertising/connection state;
- GATT/GAP capability;
- test-mode capability flag;
- link RSSI when Bluetooth is active.

The public FAP API does not currently provide a stable generic BLE advertisement scanner, so this module deliberately avoids private/internal BLE glue APIs.

## Infrared Inspector

Purpose: inspect IR traffic from equipment you are authorized to test.

Capabilities:

- receives IR signals;
- enables firmware signal decoding;
- identifies supported protocols;
- displays address, command and repeat state;
- reports raw timing count for undecoded signals.

There is no transmit call in the V2 module.

## NFC Detector

Purpose: inventory the protocol family of a presented NFC object.

Capabilities:

- uses the official greedy `NfcScanner`;
- identifies one or more supported protocols;
- reports up to four detected protocol identifiers;
- keeps results only in RAM.

It does not read application sectors, authenticate, emulate, write or modify cards.

## LF RFID Reader

Purpose: identify LF RFID credentials that the operator owns or is authorized to inspect.

Capabilities:

- automatic ASK/PSK read mode;
- protocol-name identification;
- bounded preview of up to 8 bytes;
- results kept in RAM only.

The module never calls the LF-RFID write or emulation APIs.

## Randomness utilities

Password and random-HEX tools use the firmware hardware RNG.

Password character selection uses rejection sampling to avoid simple modulo bias.
