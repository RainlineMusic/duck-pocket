## Mix / Attack revision

- Header `mix` (0–100%, default 100%) and `output` (-12…+6 dB, default 0.0 dB) use vertical numeric dragging; double-click restores defaults. Output keeps its existing ID/range and automation. Mix scales M/S component reduction in dB, without a parallel dry bus: 15 dB at 50% becomes 7.5 dB. Gain History and Influence show the resulting control reduction. In a selected frequency band this is the requested band reduction, not a broadband loudness measurement.
- Attack replaces the central Output dial: 0.0–5.0 ms, 0.1 ms steps. A finite anticipatory ramp reaches the requested reduction by the delayed key transient; it never changes the existing 5 ms latency. New instances at 0.0 ms use instantaneous aligned onset. Schema-2 and older projects retain their original fixed soft attack until Attack is changed; schema 3 stores this compatibility flag. Mix defaults to 100% in migrated projects. Old IDs/normalised automation remain intact; new parameters are appended.
- Neutral balance reads `0% / M/S`, with `mid` or `side` when turned.
- Each scope waveform supplies its own emission energy; OUT no longer modulates KEY or Gain History glow. Blur footprint is fixed, with the existing age fade retained. Bypass uses a cached half-resolution, contiguous three-pass blur instead of sparse one-sixth-resolution sampling.
- Complete zero gain has no finite dB value: intermediate Mix scales use a -120 dB floor; 100% preserves exact original silence, and 0% returns exact delayed dry before Output gain. Mix automation is smoothed and snaps back to exact endpoints.

# Duck Pocket 1.0.0

JUCE 8.0.4 sidechain VST3 / AAX by Rainline Music.

## v1.0.0

- Display-synchronised UI: 60 fps for short windows, 30 fps for 2–5 s windows.
- High-resolution 2.4 kHz graph capture and 16k history; 2 ms extrema rollups and bounded paths for long windows.
- Perspective tunnel graph grid in every theme.
- Three active themes: Solid Dark (default), Neon and Amber; old White preferences use Dark.
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

Three semantic themes, embedded Inter Regular/Medium (OFL license in `Assets/Fonts/Inter-OFL.txt`), cached physical-scale materials and dial bodies. Key/filter use one data colour; output/reduction use the second. Duration and M/S are neutral. Freeze remains available. Chrome is rebuilt only on theme, size/DPI or graph-window changes, with resize coalescing. No idle ambient animations.

Optional experimental OpenGL on macOS and Windows: configure with `-DDUCK_ENABLE_OPENGL=ON`, then use Settings → OpenGL (experimental). Runtime default is OFF, including upgrades: preference `duckPocket.ui.opengl.v3` starts false and preserves subsequent explicit choices. One editor context can present the crisp graph cores; it receives no emissive masks and performs no graph blur. Continuous repainting is disabled, static GPU textures are retained, and creation/shader failure returns to the native renderer. Context detach precedes child destruction. OpenGL is deprecated on macOS ([Apple](https://developer.apple.com/documentation/appkit/nsopenglcontext)). Windows uses the JUCE 8 WGL path as an opt-in experiment; if the host or driver rejects the context, the editor falls back to native rendering. Both platforms default to direct native graph paths. Cached dial glow is retained. Influence has no audio-driven reduction arc or audio-driven repaint. Enabling GL is not a verified performance improvement yet.

## Validation

Target platforms: **macOS universal arm64/x86_64 and Windows x64**, VST3/AAX. No Linux packages are delivered. `-DDUCK_BUILD_UI_TESTS=ON` builds processor integration tests and the real JUCE screenshot utility. Run `ctest --test-dir build -C Release --output-on-failure`. Run `PocketUITest <output-directory>` for software snapshots of all themes, three signal states, 1x/2x. Use `PocketUITest --gl-smoke` on a supported desktop for a native context, signal-driven GPU blur and 100 peer attach/detach cycles. CI records this experimental probe separately; a hosted runner may have no usable GPU. The probe does not measure Pro Tools performance.

For Debug instrumentation use `-DDUCK_SANITIZER=address`, `undefined`, or `thread` (Clang/GCC); MSVC supports the address option. GUI/host sanitizer coverage must be run on the target OS. See `IMPLEMENTATION-REPORT.md` for actual completed checks, known limitations and the remaining host/performance matrix. Developer AAX artifacts are not a production PACE-signed release.

### Designer interface

The supplied SVG is the source for the Dark theme dial material, curved outline headings and logo. Controls and traces are live JUCE components, not a screenshot overlay. The current 800 × 905 expanded / 800 × 792 collapsed layout scales with window width; previous widths are retained within 400–1500, while previous height/expanded-panel state is ignored. M/S is now a rotary control: centre **0% / MS**, left **0–100% / MID**, right **0–100% / SIDE**. The existing `msBalance` parameter remains −1…+1; double-click resets to zero. Settings, power/bypass, graph freeze, range-handle reset, themes and graph windows are preserved.

Ring bloom is cached by value, theme, size and physical scale and works with native rendering, including Windows. Experimental OpenGL is optional on both platforms and off by default, with automatic fallback. This visual update does not modify DSP or parameter state.

### Compact luminous interface update

Active themes: Solid Dark (default), Neon and Amber. The removed White preference migrates to Dark. Fresh windows default to width 615 (approximately 1/1.3 of the designer base), with a 400 minimum (half the old 800); height follows the same proportions. Open filters use aspect 800:905, collapsed aspect 800:792. The new UI preference defaults the filters closed. The restored arrow hides both Sidechain Filter and Processing Range, preserving width and parameter values. Previously saved session widths within bounds remain valid.

OpenGL is compiled by default and disabled by default at runtime on both platforms. Runtime choice/failure rollback is saved under preference v3; previous v2 defaults are ignored. Windows GL remains opt-in because WGL stability varies between hosts and drivers. Fresh processors already default to Percentage Duration at 100% = AUTO. Old ms-mode session states retain their saved mode.

Graph beds use cached dark recesses and inner bevels. Curve cores fade towards older data using cached horizontal alpha gradients; graph bloom is permanently disabled. Output values rounding to zero show 0.00, never -0.00. Freeze/resume cut-offs are discarded on host reset, and GPU publications request their own presentation so paused theme changes do not wait for audio.

### Photoshop layout refinement

The supplied 1894×2048 comparison defines the layout offsets: side dials up 22 reference pixels; centre dials down 49; logo right/down 19/3. Freeze moves to the bottom-right of the oscilloscope. Existing control sizes, graph geometry and parameter semantics are preserved. The chevron is 1.5× larger with static glow, sits lower, and has more footer room. New default size is 615×609 closed / 615×696 open; minimum width is still 400. First launch/upgrade defaults closed; explicit subsequent choices persist. Dark has cooler, deeper materials and stronger live/cached bloom.

Height-only panel toggles reuse the chrome cache and preserve width. The stale-image stretching and intermediate constrained resize are eliminated; GL uses the live drawable viewport during the transition. Regression screenshots compare stable paused top pixels through repeated folds at minimum/default/maximum width. DSP and processor sources are unchanged by this update.

### Windows playback and graph glow

Graph glow is permanently OFF on both platforms. Its menu item and preference reader have been removed, so old saved ON settings cannot re-enable it. Both plot fills and strokes use horizontal alpha gradients from 8% of their normal opacity at the oldest edge to full opacity at NOW. Six gradients are built only on a theme change, in reference coordinates; resize/DPI scaling uses the existing Graphics transform. Fading adds no draw pass, blur, history accumulation, pixel loop or offscreen image. Native rendering draws both graphs directly; optional macOS GL only presents the crisp cores. Dial/button materials are unchanged. The Windows native smoke test exercises three simultaneous editors with live audio at 1x/2x and asserts that no software bloom layer is prepared. The Mac native probe asserts that no GPU blur runs. Full-snapshot timings are diagnostics, not DAW CPU/GPU measurements.
