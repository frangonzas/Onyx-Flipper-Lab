# Security Policy

## Scope

Security reports concerning Onyx Flipper Lab are welcome.

This project is intended for defensive engineering, education and explicitly authorized hardware assessment.

## Responsible disclosure

Please avoid publishing sensitive exploit details before remediation can reasonably be assessed.

A useful report should include:

1. affected version or commit;
2. reproducible steps;
3. expected impact;
4. non-destructive proof using owned or authorized hardware;
5. suggested mitigation, if known.

Never include real credentials, private keys, tokens or personal data in a public issue.

## V2 operational boundaries

The current audit modules intentionally exclude:

- Sub-GHz transmission or replay;
- infrared transmission;
- NFC emulation or write operations;
- LF-RFID write/emulation;
- brute force;
- jamming/interference;
- Bluetooth profile replacement or bonded-device deletion;
- hidden persistence of observed identifiers.

GPIO auditing is input/ADC-only.

Observed identifiers and generated values are held in application memory and are not intentionally written to storage.

## Authorization

This project does not authorize testing third-party systems, credentials, locks, access cards, radios, vehicles, alarms or infrastructure without explicit permission.

## Safe-harbor intent

Good-faith research performed on systems and hardware you own or are explicitly authorized to assess, without disruption or unnecessary data access, is considered responsible research within this project.
