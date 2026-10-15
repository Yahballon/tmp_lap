# PS5 System Info v0.1.0

First native PS5 diagnostic dashboard release.

## Features
- Original dark-blue 1920x1080 dashboard and custom silicon-chip launcher icon
- 14 controller-navigable sections, homebrew discovery, network interface route detection
- Read-only process telemetry, local timestamps and monotonic uptime
- Persistent settings and exportable plain-text system report
- Clear unavailable state for privileged or unsupported console metrics

## Installation
Extract `PPSA99786.zip` to `/data/homebrew/`; register with compatible homebrew loader.

## Known limitations
- Some system-only hardware readings require verified firmware-specific providers.
- Only homebrew applications under `/data/homebrew` are enumerated.
- PS5 hardware validation is pending; GitHub Actions checks compilation only.
