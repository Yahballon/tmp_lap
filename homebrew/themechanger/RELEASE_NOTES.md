# ThemeChanger v0.3.18 - Signal LED diagnostics

- Dedicated /data/ThemeChanger/signal.log (plus rotated signal.old.log); Signal events remain in themechanger.log too.
- Log requested blue/white/amber values, preview state and offline command blocking.
- Assign a request ID to every service request and log its method, route, JSON body and HTTP response. ELF payload bytes are never logged.
- After accepted colour commands, read /status back on the same worker before the next colour command runs. Log requested versus reported values, LED availability, colour availability, brightness, active lighting, held colour state and service version.
- Distinguish matching values, differing values, unavailable colour control and failed readback. Reported service values do not prove a physical LED change.
- Validate the diagnostic classifications alongside the existing transport and integration tests.

To investigate: open Signal Companion, adjust Blue/White/Amber or select a preset, wait a few seconds, then retrieve /data/ThemeChanger/signal.log and signal.old.log if present. Firmware and console model are useful alongside the log. This release adds diagnosis; it does not claim to fix an unconfirmed hardware-level LED fault.
