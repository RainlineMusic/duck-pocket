# Duck Pocket 1.0.0

JUCE 8.0.4 sidechain VST3 / AAX by Rainline Music.

## v1.0.0

- Display-synchronised UI: 60 fps for short windows, 30 fps for 2–5 s windows.
- High-resolution 2.4 kHz graph capture and 16k history; 2 ms extrema rollups and bounded paths for long windows.
- Perspective tunnel graph grid in every theme.
- New warm Amber theme alongside Neon, Solid Dark and Solid White.
- Click-free 2.5 ms latency-aligned bypass crossfade.
- Duration automation follows an active envelope with smoothing.
- Both audio channels are represented in the oscilloscope.
- Dynamic filters use parked fast paths; the processor uses pointer-based block access.
- High-DPI chrome uses the actual graphics-context scale and is not regenerated while range handles are dragged.
- M/S percentages, centred range titles, edge-aligned live frequency labels, and an always-visible three-column control row.
- Old expanded-panel flags are ignored. Active parameter IDs, ranges and ordering are preserved; new percentage controls are appended.

See `PERFORMANCE-VALIDATION.md` for the Pro Tools macOS validation procedure and `V010-NOTES.md` for version history.

## Processing

A 5 ms lookahead soft-attack ducker. Influence 0–100 is linear depth; 100–150 is exponential. M/S balance changes processing depth, not output level. The key filter is a non-resonant 12 dB/oct HP+LP; full-range endpoints bypass it.

New instances use **Duration 1–100%, default 100% (AUTO)**. 100% exactly retains the original AUTO detector/envelope. Below 100%, the plugin measures the last completed key event and uses that duration for the next event: 50% halves the measured length, with a 5 ms minimum and the existing half-hold/half-cosine release. A new onset also closes the preceding measurement. The first event after prepare/reset uses AUTO until a length is known. Changes are smoothed and apply to the active envelope. Irregular hits, long tails and legato signals therefore cannot promise an exact fraction of the current unknown event.

Old sessions retain millisecond Duration and its automation (5–2000 ms; the maximum is AUTO). Settings → Percentage Duration switches modes. The old `duration` ID/range is retained; `durationPercent` and `relativeDuration` are appended. State schema 2 preserves the selected mode and reads legacy XML. Window width is retained; obsolete expanded-panel state is ignored.

Processing Range is a subtractive dynamic bell/shelf. The dry path is never permanently filtered; with no reduction the output is latency-aligned dry.

## Builds

GitHub Actions builds macOS universal arm64+x86_64 and Windows x64 VST3/AAX packages, runs pluginval at strictness 5, and executes the current DSP tests.

This is experimental software. Back up old projects and plug-ins before replacement.

## Graphics

Four semantic themes, embedded IBM Plex Sans/Mono (OFL license in `Assets/Fonts`), cached physical-scale materials and dial bodies. Key/filter use one data colour; output/reduction use the second. Duration and M/S are neutral. Freeze remains available. Chrome is rebuilt only on theme, size/DPI or graph-window changes, with resize coalescing. No idle ambient animations.

Optional OpenGL on macOS: configure with `-DDUCK_ENABLE_OPENGL=ON`, then Settings → OpenGL (experimental). A single editor context renders a half-resolution emissive layer with separable blur and additive composition. Continuous repainting is disabled, static GPU textures are retained, and creation/shader failure returns to the native renderer. Context detach precedes child destruction. **Runtime default is off until Pro Tools/AAX profiling and host testing are complete.** OpenGL is deprecated on macOS ([Apple](https://developer.apple.com/documentation/appkit/nsopenglcontext)). Windows uses JUCE 8's native Direct2D renderer: the OpenGL setting is unavailable there because a hosted WGL peer crashed intermittently before the renderer could apply its fallback. Enabling GL on macOS is not a verified performance improvement.

## Validation

Target platforms: **macOS universal arm64/x86_64 and Windows x64**, VST3/AAX. No Linux packages are delivered. `-DDUCK_BUILD_UI_TESTS=ON` builds processor integration tests and the real JUCE screenshot utility. Run `ctest --test-dir build -C Release --output-on-failure`. Run `PocketUITest <output-directory>` for software snapshots of all themes, three signal states, 1x/2x. Use `PocketUITest --gl-smoke` on a supported desktop for a native context, signal-driven GPU blur and 100 peer attach/detach cycles. CI records this experimental probe separately; a hosted runner may have no usable GPU. The probe does not measure Pro Tools performance.

For Debug instrumentation use `-DDUCK_SANITIZER=address`, `undefined`, or `thread` (Clang/GCC); MSVC supports the address option. GUI/host sanitizer coverage must be run on the target OS. See `IMPLEMENTATION-REPORT.md` for actual completed checks, known limitations and the remaining host/performance matrix. Developer AAX artifacts are not a production PACE-signed release.
