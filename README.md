# Stud — Roblox Player for Linux

A Linux-only Roblox Player launcher with a native Qt 6 interface. Based on
[CatPieLeaf's Stud](https://github.com/CatPieLeaf/Stud), with a redesigned Player
home, searchable settings categories, animated controls and skeleton loading.
Studio is not offered by this fork.

## Download

**The AppImage from [this repository's releases](https://github.com/vivo6810/stud-with-better-ui-/releases/latest)
is the only supported launcher download.** No release published yet? Use an
AppImage artifact from a successful [AppImage workflow run](https://github.com/vivo6810/stud-with-better-ui-/actions/workflows/release.yml).
Artifacts require a GitHub login and expire according to GitHub retention settings.

Download `Stud-x86_64.AppImage`, then:

```sh
chmod +x Stud-x86_64.AppImage
./Stud-x86_64.AppImage
```

Only run builds you trust. This fork's release checks point to this repository, not the upstream project's
binaries. Update notifications open the release page for manual download.

## Interface

- Player-only home with keyboard-accessible Play, Settings and release notes.
- Existing desktop palette, flat surfaces, no glow or gradients; a blue Play accent.
- Stretch/bounce interactions and smooth page reveals.
- Reduce motion in General settings; restart to stop all existing animations.
- Pulsing solid skeletons for pending work, actual download progress when known,
  slow-loading hints, cancellation and retry for release notes.
- Existing graphics, customization, history, FFlags and diagnostics retained.

Settings categories can be filtered by name. All launcher preferences, including reduced motion, use **Save changes**.

## How it works

Stud downloads Roblox's unmodified **macOS Player** from Roblox's servers and runs
it on **Linux through Rosemary**. This is not a Windows or macOS launcher. Roblox
and the runtime's required downloads are separate from downloading Stud itself.
No Roblox client is distributed in this repository.

## Build the AppImage

The AppImage workflow runs on pull requests, main and Arena branch pushes, manual
runs, and published releases. Successful runs prepare `Stud-x86_64.AppImage` and `sourcecode.zip` (the matching
tracked source, without build dependencies). Published releases attach these two
files as direct downloads; the AppImage is not wrapped in a ZIP. GitHub also
adds its own automatic source ZIP/tar.gz links, which cannot be removed.

Actions artifacts are still downloaded as ZIP containers by GitHub. For a direct
AppImage download, use the **Releases** page after a release build succeeds.

Locally, with Docker or Podman installed:

```sh
packaging/build-appimage.sh
```

Outputs are written to `build-appimage/packages/`. The existing Arch-based build
bundles its dependencies. Internet access is required for build dependencies,
Rosemary, and AppImage tooling. See `packaging/Containerfile.appimage` for the
build environment. Native development uses CMake, Ninja, Qt 6 Widgets/DBus/Network,
OpenSSL, libsecret and the Rosemary build dependencies from that container file.

## Licensing and attribution

See [LICENSE](LICENSE), [LICENSE.exception](LICENSE.exception) and [NOTICE.md](NOTICE.md).
Original Stud by CatPieLeaf; upstream attribution is retained. This fork is not
an official Roblox product. See [docs/ui-revamp.md](docs/ui-revamp.md) for validation.
