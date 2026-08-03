# Linux feature checklist

This list tracks parity with OpenKey for macOS. Items marked **done** are
implemented in the Linux IBus frontend; the remaining work is ordered by user
impact.

## Typing and top-bar controls

- [x] IBus engine for Ubuntu Wayland/X11
- [x] VI/EN top-bar indicator and Vietnamese/English toggle
- [x] Input type selection: Telex, VNI, Simple Telex 1/2, Custom
- [x] Custom 11-key typing layout
- [x] Code-table selection: Unicode, TCVN3, VNI Windows, Unicode Compound, CP1258
- [ ] Per-application input-mode and code-table memory
- [ ] Configurable keyboard shortcut to toggle Vietnamese/English
- [ ] Temporary-disable shortcuts and compatibility profiles

## Control panel

- [x] Vietnamese four-tab control panel: Typing, Macros, System, About
- [x] Core spelling, orthography, Quick Telex, consonant, and macro toggles
- [ ] Macro editor: list, add, edit, delete, import/export
- [ ] Conversion-tool dialog and quick-convert shortcut
- [ ] Startup toggle that creates/removes the user autostart entry
- [ ] Reset-to-defaults and richer About/version dialog

## Packaging and quality

- [x] CMake build and install layout
- [ ] Debian package with dependencies and install hooks
- [ ] Autostart desktop entry
- [ ] Automated engine tests, including Terminal regression tests
