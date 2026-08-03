# OpenKey for Linux

OpenKey for Linux is an IBus Vietnamese input-method engine with a GTK control
application. It works in Ubuntu's GNOME session on both Wayland and X11.

The control application displays a `VI`/`EN` indicator in the top bar. Its menu
toggles Vietnamese mode and opens settings for Telex/VNI, spell checking,
modern orthography, Quick Telex, macro support, and other engine options.

## Build and install

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
sudo glib-compile-schemas /usr/share/glib-2.0/schemas
ibus restart
```

Add **Vietnamese - OpenKey** through *Settings → Keyboard → Input Sources*.
Run `openkey-control` once to show the top-bar menu; add it to Startup
Applications if you want it to launch automatically.
