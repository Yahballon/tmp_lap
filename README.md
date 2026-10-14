# PS5 Homebrew Projects

This repository hosts **multiple independent native PlayStation 5 homebrew applications**. Each app keeps its source code, build assets, and documentation in its own folder under `homebrew/`. GitHub Actions workflows remain in the repository-wide `.github/workflows/` directory (as GitHub requires).

## Projects

| Application | Source | Build | Release |
| --- | --- | --- | --- |
| Torrent Flow | [homebrew/ps5-torrent](homebrew/ps5-torrent/) | [Build Torrent Flow](.github/workflows/build-ps5-torrent.yml) | [v0.1.4](https://github.com/Yahballon/tmp_lap/releases/tag/ps5-torrent-v0.1.4) |
| Crypto Tracker | [homebrew/crypto-tracker](homebrew/crypto-tracker/) | [Build Crypto Tracker](.github/workflows/build-ps5-crypto-tracker.yml) | [v0.3.0](https://github.com/Yahballon/tmp_lap/releases/tag/v0.3.0) |
| WWW Speedtest | [homebrew/www-speedtest](homebrew/www-speedtest/) | [Build WWW Speedtest](.github/workflows/build-www-speedtest.yml) | [v0.4.3](https://github.com/Yahballon/tmp_lap/releases/tag/www-speedtest-v0.4.3) |
| Starbound Sprint | [homebrew/starbound-sprint](homebrew/starbound-sprint/) | [Build Starbound Sprint](.github/workflows/build-starbound-sprint.yml) | [v0.5.0](https://github.com/Yahballon/tmp_lap/releases/tag/starbound-sprint-v0.5.0) |
| Payload LauncherX | [homebrew/payload-launcher](homebrew/payload-launcher/) | [Build Payload LauncherX](.github/workflows/build-payload-launcher.yml) | [v0.2.4](https://github.com/Yahballon/tmp_lap/releases/tag/payload-launcher-v0.2.4) |

## Layout

```text
tmp_lap/
├── .github/
│   └── workflows/
│       └── build-ps5-crypto-tracker.yml
├── homebrew/
│   └── crypto-tracker/
│       ├── native/
│       │   ├── main.cpp
│       │   ├── market_client.cpp / .hpp
│       │   ├── pin_settings.cpp / .hpp
│       │   ├── coin_logos.cpp / .hpp
│       │   ├── generate_icon.py
│       │   └── generate_coin_atlas.py
│       └── README.md
├── LICENSE
└── README.md
```

To add a new homebrew app, create a separate `homebrew/<app-slug>/` folder with its own native sources, metadata and docs, plus a dedicated workflow under `.github/workflows/`. Give every app a unique Title ID and an application-specific release tag, such as `my-app-v0.1.0`. Set each workflow's `push.paths` and `pull_request.paths` filters to its own app directory and workflow file.

**Crypto Tracker installation is unchanged:** copy the built `PPSA99442` folder to `/data/homebrew/` on your console. This repository refactor does not change the PS5 package or its already-published `v0.3.0` release.
