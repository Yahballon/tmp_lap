# Hallo IPA

A minimal native **SwiftUI iPhone test app**. When launched, the screen displays **Hallo IPA** centered both vertically and horizontally.

This sample lives in `ipa_tests/hello-ipa/` and does not affect the PS5 homebrew projects in this repository.

## Build

- **Source:** `Sources/HalloIPAApp.swift` and `Sources/ContentView.swift`
- **XcodeGen configuration:** `project.yml` (Xcode project is generated on macOS)
- **GitHub Actions:** [Build Hallo IPA](../../.github/workflows/build-hello-ipa.yml)
- **Minimum iOS version:** iOS 15
- **Bundle identifier:** `com.yahballon.ipatests.helloipa`

Every push to `main` that changes this app (or its workflow) builds an unsigned device IPA on a GitHub-hosted macOS runner. The workflow uploads `Hallo-IPA-unsigned.ipa` as an Actions artifact and creates a tagged **prerelease** with that IPA.

You can also start a build manually via **Actions → Build Hallo IPA → Run workflow**. Manual runs only upload an Actions artifact and do not create a release.

## Install on a normal iPhone (Windows)

1. Download `Hallo-IPA-unsigned.ipa` from the latest **Hallo IPA** prerelease or from the workflow artifacts.
2. Connect your iPhone to your Windows PC, enable Developer Mode if iOS requests it, and open [Sideloadly](https://sideloadly.io/).
3. Select the IPA, sign it with your own Apple Account, and install it on the device. Follow device trust/developer prompts.
4. Launch **Hallo IPA**; the text is centered on the screen.

**Important:** The distributed IPA is **unsigned**. It cannot install directly until it is signed with a valid certificate and provisioning profile. A free Apple Account can be used for personal testing with limitations, including typically needing to re-sign after seven days. No Apple ID, password, certificate, or other private signing material is required in this public repository.

## Build with Xcode locally (macOS)

```bash
brew install xcodegen
cd ipa_tests/hello-ipa
xcodegen generate
open HalloIPA.xcodeproj
```

Use your own signing team in Xcode when running directly on a physical iPhone.
