# Embedded engineering notes

## Runtime boundaries

- Target Qt 5.15 with Qt Widgets. Qt 6 Multimedia has a different API and is not a drop-in replacement.
- Use one native application window. Feature pages are embedded in a `QStackedWidget` and created on first use.
- UI code lives in page classes; `Core/` owns configuration, hardware state and asynchronous services.
- Camera and sensor data are real device readings. An unavailable device produces an explicit unavailable state.

## I/O and lifetime

- Network operations use HTTPS, normal certificate validation, bounded timeouts and reply cleanup.
- Use asynchronous D-Bus for ConnMan. Passwords must never enter shell commands or logs.
- Download tasks own their names, paths and `QSaveFile`; stream data to disk and commit only valid audio.
- Serial and camera workers use nonblocking descriptors with bounded polling. Stop workers before releasing their resources.
- Camera frames are copied before V4L2 buffer reuse; at most one frame waits for the UI.
- Only static application assets belong in `.qrc`. Media, credentials and generated data remain external.

## Display and deployment

- The application targets 1024 × 600. Preserve at least 48-pixel touch targets for primary controls.
- Keep interface colors in `Ui/theme.qss` and shared line icons in `Ui/uikit.cpp`.
- Bind interface text through `Core/i18n.h`; English strings live in `Translations/en_US.json` and Chinese is the first-run default. Literal user data uses `setLiteralText`, and `setPixmap` clears stale placeholder bindings before displaying images.
- Compressed JPEG and PNG file sizes do not predict decoded pixmap memory use. Measure resident memory on the board.
- `hide()` does not free a page. Pause video/camera and hidden-page polling explicitly.
- Keep board-specific device nodes and service credentials in `vehicle.ini` or environment variables.

## Verification

Run `tests/tests.pro` for GPS parsing, weather validation, voice matching, downloads and shell lifecycle tests.
Use `--screenshots <directory>` to capture actual application pages. Hardware-off screenshots intentionally show unavailable states.
On the target, verify V4L2 format support, ALSA/GStreamer codecs, Qt TLS support, framebuffer/touch input,
ConnMan permissions, GNSS serial settings, DHT11 driver behavior and repeated page switching.
