# Architecture

## Design goals

Onyx Flipper Lab v1 prioritizes:

1. a small attack surface;
2. predictable input handling;
3. no hidden persistence;
4. no network or radio transmission;
5. no active GPIO manipulation;
6. clear ownership and authorization boundaries.

## Runtime model

The application uses a simple event-driven architecture.

```text
Flipper buttons
      │
      ▼
input callback
      │
      ▼
FuriMessageQueue
      │
      ▼
main app loop ──────► protected state
                         │
                         ▼
                    draw callback
                         │
                         ▼
                       GUI
```

The input callback does not perform application logic. It copies the input event into a Furi message queue.

The main application loop owns navigation and state changes.

The GUI draw callback reads the same state under a mutex.

## Randomness

Random values come from the Furi HAL hardware RNG API.

The password generator uses rejection sampling instead of a direct modulo operation when selecting characters, reducing modulo bias.

Passwords are generated in volatile application memory. v1 does not intentionally write them to the SD card or transmit them.

## Screens

### Main menu

Owns navigation only.

### Password Generator

Generates an 18-character password using a restricted character set that avoids some visually ambiguous characters.

### Random HEX

Displays 8 random bytes as hexadecimal.

### Security Tips

Static, defensive security guidance.

### GPIO Safety

Read-only safety guidance. No GPIO output function is called.

### About

Project metadata.

## Threat model

The v1 threat model is intentionally narrow.

### Assets

- generated random values;
- application integrity;
- user expectation that the app is passive.

### Trust boundaries

- physical input from the user;
- Flipper firmware / Furi APIs;
- GUI service.

### Non-goals

v1 is not designed to:

- protect secrets after the device itself is compromised;
- replace a dedicated password manager;
- provide cryptographic key storage;
- interact with third-party RF/NFC/IR systems;
- automate penetration testing.
