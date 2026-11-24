# Player UI revamp

The launcher home now opens Roblox Player only. Studio command-line requests,
Studio deep links and place files are rejected before launch. Studio's settings
tab and installed desktop action are no longer exposed. Shared runtime and
configuration code are retained to avoid deleting existing user data.

The interface uses the desktop's existing Qt palette, solid surfaces, and a
small blue Play accent. There are no new gradients, glows or shadows. The Play
control stretches inside its stable hit target with an OutBack bounce; content
reveals use an OutCubic opacity transition. Loading skeletons pulse solid fills
and stop their animation when hidden.

Skeletons represent real pending work, not fabricated game cards. Release notes
load asynchronously, display a slow-connection hint, and offer Retry on failure.
The launch screen switches between skeletons for unknown progress and measured
download progress, retaining cancellation. A prolonged operation is not treated
as proof of a network failure.

## Validation still required on Linux with Qt 6

- Configure and build using the README's dependencies and run CTest.
- Open Home, Settings and What's new; verify keyboard navigation and scaling.
- Play and cancel; retry after cancellation and verify a new launch can succeed.
- Throttle networking: observe skeletons, slow hint, timeout and Retry in notes.
- Disconnect networking: verify launching reports the runtime's existing error.
- Test --studio, roblox-studio links and .rbxl/.rbxlx rejection.
- Install desktop entries and verify only Player and Settings actions appear.
- Exercise light/dark palettes and fractional scaling.

The settings interface now has shared flat styling, a category filter, generous
spacing, page reveals, and a persistent reduced-motion preference in General.
The preference is stored separately in Qt's Stud/Interface settings after a successful Save; restart
for all already-running animations to stop. Existing runtime settings and data
formats are preserved.

The AppImage workflow builds PRs, branch pushes, manual runs and releases;
only a release event can publish. All launcher update checks point to this fork; updates open its release page.
Releases attach only the standalone AppImage and matching sourcecode.zip.
No zsync metadata is published or requested. Legacy package recipes remain as
reference but the release workflow no longer publishes those formats.

Static checks are recorded in the implementation response. No compiled or visual
validation was possible because CMake and Qt tooling are absent here. The Linux
validation checklist above must pass before release.

## Follow-up fixes

Reduced motion participates in Reset all and right-click reset. Its saved value
is cached for rendering. The loading screen reserves skeleton space, uses one
cancel path for its button/Close/Escape, and keeps a persistent elapsed-duration
hint rather than resetting it on every progress report. Release notes try both
bare and v-prefixed version tags on a missing-tag response. Download writes
reject short writes before commit. The offscreen loading lifecycle regression
is part of AppImage CI; native and visual verification remain required.
