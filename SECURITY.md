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

## v3 operational boundaries

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

## Report persistence

v3 can write one user-triggered field report to app storage.

The report contains aggregate measurements and event counters only.

It deliberately does not save:

- NFC UIDs;
- LF RFID payload bytes;
- IR address/command values;
- generated passwords;
- replayable RF captures.

The latest report replaces the previous report to limit data accumulation.

## Authorization

This project does not authorize testing third-party systems, credentials, locks, access cards, radios, vehicles, alarms or infrastructure without explicit permission.

## Safe-harbor intent

Good-faith research performed on systems and hardware you own or are explicitly authorized to assess, without disruption or unnecessary data access, is considered responsible research within this project.
