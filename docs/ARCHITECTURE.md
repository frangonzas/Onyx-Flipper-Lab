# Architecture

## Design goals

Onyx Flipper Lab v2 prioritizes:

1. passive observation before active testing;
2. public/stable Flipper SDK APIs;
3. bounded in-memory data handling;
4. explicit module lifecycle and cleanup;
5. no hidden persistence;
6. no replay, cloning, jamming or brute-force paths.

## Runtime model

```text
buttons
  │
  ▼
input callback
  │
  ▼
FuriMessageQueue
  │
  ▼
main event loop ─────────► module lifecycle
  │                           │
  ▼                           ├── GPIO ADC
mutex-protected state         ├── Sub-GHz RX
  │                           ├── Bluetooth HAL
  ▼                           ├── IR worker
GUI draw callback             ├── NFC scanner
                              └── LF RFID worker
```

The input callback only queues events. Hardware start/stop operations are performed by the main application thread.

## Concurrency

IR, NFC and LF-RFID use firmware workers/callbacks.

Callbacks:

- copy only bounded results;
- do not perform UI rendering;
- take the application mutex before modifying shared state;
- do not persist credential material.

The GUI draw callback uses the same mutex for consistent snapshots.

## Module lifecycle

When entering a hardware module, the main thread allocates/acquires only the resources needed by that screen.

When BACK is pressed:

1. the screen is switched back to the menu;
2. the corresponding worker/scanner/radio resource is stopped;
3. allocated objects are freed/released.

This prevents hardware resources from remaining active after the user leaves a module.

## Sub-GHz

The internal CC1101 is:

1. initialized;
2. reset;
3. configured with the official asynchronous OOK preset;
4. tuned to a selected hardware-supported frequency;
5. placed into RX;
6. sampled for RSSI/LQI.

No TX function is called.

## GPIO

The ADC auditor uses firmware GPIO metadata to locate ADC-capable, non-debug pins.

The selected pin is placed into analog input mode, sampled and converted to millivolts. It is never driven high or low.

## Bluetooth

The public FAP API currently provides controller/link posture APIs but no stable generic passive BLE advertisement scan interface. V2 therefore avoids private BLE stack internals.

## Threat model

Primary assets:

- integrity of the Flipper runtime;
- operator expectation that radio modules are passive;
- transient tag/signal observations;
- generated random values.

Primary trust boundary:

- external signals and tags are untrusted inputs.

Non-goals:

- attacking third-party systems;
- bypassing access control;
- extracting protected NFC application data;
- cloning credentials;
- replaying radio captures;
- long-term credential storage.
