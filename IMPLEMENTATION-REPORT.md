# Duck Pocket — implementation and verification

Base: Release-1.0, `4f20dbd1a7b2d2ff245f05626878f3098adec438`.
Target delivery: macOS universal + Windows x64; VST3/AAX. No Linux package or CI job was added.

## Implemented

- Sidechain expansion arrow removed. Filter, Processing range and Mid/Side always visible in three aligned columns. Fixed-aspect resize retained, width 800–1500; obsolete expansion state ignored.
- Equal-width Gain History/Oscilloscope, common grid and axis labelling. Real OUT/KEY legend, no decorative header dots or empty three-dot menus. Freeze retained; icons have tooltips.
- Percentage Duration defaults to 100% = original AUTO. Separate appended host parameters preserve old IDs, ranges and automation. Old states select the old ms mode. Schema 2 sanitises nonfinite/out-of-range stored values.
- Four semantic palettes and OFL IBM Plex Sans/Mono embedded, including font license. Cached textured chassis, directional panel lighting, inset plot glass and matching machined dial bodies. Output shares the dial implementation. Duration/AUTO remains neutral, with an unfilled track. Influence shows the actual reduction ring.
- Static material and dial images use the graphics context's physical scale. Resizes are coalesced; control dragging does not rebuild chrome. Dynamic graphs fade with age; grid recedes into the tunnel. Gain fill follows actual reduction. Quarter-resolution software emissive buffers use separable binomial blur and additive RGB over cached glass, with crisp cores last. Scope history accumulates a time-decayed, scrolling phosphor trace in the software renderer. NOW flashes for 180 ms on a rising reduction edge. Angular satin highlights and blurred shadows are baked into static images. Short hover/drag transitions stop their timer when settled. Silence/freeze avoid graph repainting after old data leave the window.
- Optional one-context GL backend: half-resolution emission, horizontal/vertical Gaussian shader passes, additive composition, level-dependent radius/intensity, retained static texture, no continuous repaint, safe detach, timeout/shader fallback. Runtime default OFF. Windows native Direct2D remains the baseline.
- Six regression/integration CTest entries; CI configured only for Mac/Win, including all tests, UI captures and existing pluginval strictness 5. A separate Mac address/undefined + thread sanitizer matrix builds the software renderer and exercises the processor/concurrent trace exchange and all theme captures; configuration is not a successful result.

## Duration: exact meaning

At 100% the output is bit-identical to the old AUTO. Below 100%, the requested fraction applies to the **previous measured event**, then retains the original half-hold/half-cosine release with a 5 ms minimum. First event after reset/prepare uses AUTO. Measurement uses the detector's filtered key, a -60 dB-relative audible threshold and its existing event boundaries; not a DAW clip boundary. Consecutive onsets close the previous measurement. Percentage automation is smoothed.

With only 5 ms lookahead, the duration of a future arbitrary event cannot be known. Different/legato/sustained events may not produce an exact fraction of the currently arriving sound. This is a functional limitation, not a claim of predictive detection. Legacy `duration` automation is never reinterpreted as percentages.

## Bug audit

Line references are to this branch; the functions identify the corresponding original location.

| Location | Severity | Reproduction | Fix / regression evidence |
|---|---|---|---|
| `Source/PocketDSP.h:31`, `Engine::process` | high | Nonfinite or enormous finite input overflows M/S/filter arithmetic; state can remain poisoned after corrupt input | Reject NaN/Inf and absurd >1e12 input before arithmetic; `numerical_test` covers recovery/extremes. Ordinary finite audio is unchanged |
| `Source/PocketDSP.h:32`, `Engine::reset/latencyForRate` | medium | Invalid rate is converted to a delay length or used in coefficients; invalid reset amount persists | One finite, bounded rate policy and cleaned amount; numerical tests |
| `Source/PluginProcessor.cpp:57,103` | medium | Invalid host SR still reaches float-to-int trace decimation or timestamp division although DSP reset is sanitised | Use the same validated rate for capture; processor test includes zero/NaN/Inf rates |
| `Source/PluginProcessor.cpp:60,68,117`; editor `frameTick` | medium | Reset/prepare restarts trace time while previously queued/editor packets still belong to the preceding timeline; graph can interpolate across epochs | Tag packets with an atomic generation; discard old epochs and reset UI rollups/clock. FIFO is not reset against a concurrent reader. Reset/concurrent-exchange tests |
| editor VBlank callback / `frameTick` | medium, UI | Original timing thresholds allow >60/30 fps on high-refresh displays and continued silence redraw | Display-driven deadline with 60/30 budget; skip settled silent graphs. Host frame/CPU profiling remains pending |
| state load/save, layout | compatibility | Old expanded flag/window shape conflicts with permanent lower controls | Ignore flag, preserve/clamp width and derive height. Old XML + roundtrip tests |
| font/UI layout | low | Small system-font values and unrelated decorative accents reduce legibility | Embedded fonts, minimum design labels scaled to ≥11 logical px at minimum width, semantic roles; capture audit |

The audio path has no new allocation, UI, I/O or renderer calls. Delay/peak storage is allocated on reset/prepare. Capture uses the existing bounded SPSC AbstractFifo, with one producer/consumer. Audio samples/filter/envelope calculations are unchanged except the explicit invalid-value guard and authorised percentage mode. Integer decimation remains nominal 2.4 kHz (2450 Hz at 44.1 kHz); 16,384 raw history and 4096 × 2 ms extrema rollups remain. This deliberate preservation avoids changing trace/audio behaviour during the redesign.

Main buses remain mono/stereo; sidechain disabled/mono/stereo. Unsupported multichannel layouts are rejected. M/S in mono processes Mid. Bypass retains the original latency-aligned 2.5 ms crossfade. Latency tests cover 44.1/48/88.2/96/176.4/192 kHz. Processor blocks cover 0,1,3,17,128,4096,65536 samples.

## Completed local checks

The development environment is not Mac/Win. It was used for source compilation, offscreen JUCE captures and portable regression checks only.

| Check | Actual result |
|---|---|
| Debug build, JUCE 8.0.4, GL option ON | compiled VST3, processor test and capture utility |
| CTest | 6/6 passed: V10, short Duration, numerical, percentage Duration, frozen-baseline null, processor integration |
| Frozen-baseline null | bit-exact across six SRs, four legacy durations, noise/hits, changing Influence, M/S, processing range, output and bypass |
| New 100% | bit-exact AUTO comparison at 44.1/48/96/192 kHz |
| New 25/50/75% | learned 100 ms event ends at requested fraction after latency/release margin |
| ASan + UBSan | numerical and percentage tests passed; no findings |
| LeakSanitizer | unavailable: sandbox denies `/proc/.../task`; no successful leak-sanitizer claim |
| State/layout/lifecycle | old XML migration, broken/null state, roundtrip, buses, variable blocks, latency, trace resets, 100 editor create/destroy/resize cycles passed |
| SPSC stress | real processor producer + trace consumer concurrently exercised, including reset; finite and ordered records checked |
| UI captures | all four themes × silence/quiet/strong × 1x/2x, real processor-driven traces; warm chrome reuse assertion passed |
| Contrast | secondary/chassis: Neon 8.48, Dark 7.55, White 5.31, Amber 7.95; secondary/raised ≥5.76. Cached lighting is restrained; check native text rasterisation on target DPI |
| Native GPU/host validation | not verified locally; offscreen snapshots do not exercise GL or target native DPI changes |

Full-window Debug software snapshots averaged about 71.7 ms after warming the cache in this restricted environment. Theme/DPI cache construction + capture took roughly 170–1250 ms. These are offscreen snapshot timings, **not** native DAW frame time or CPU/GPU utilisation; they do not establish 60 fps. The existing native editor/pluginval attempt on the local virtual display failed to create a valid X11 window, so it is not presented as a pluginval pass or as a Mac/Win diagnosis. Target pluginval is delegated to the Mac/Win CI.

## Anti-AI design audit

- Removed rainbow roles, decorative header dots, empty menus, permanent luminous dial bodies and unrelated pseudo-meter detail.
- Hardware detail is tied to construction: machining, tick marks, inset glass, borders, one top-left light. Real reduction supplies the extra Influence ring.
- One key colour and one output/reduction colour across control and graph. Duration/M/S use neutral values. No fake readings or generated demo labels: snapshots process actual synthetic kick/bass audio.
- Silence has no emissive bloom. Quiet signals produce restrained curves; strong reduction supplies deeper fill and activity. Frozen curves do not keep software bloom. Hover/drag transitions stop after settling; NOW flashes have a final clear frame even if audio stops.
- Four-theme captures were inspected. The graph/Influence hierarchy survives reduction. White stays off-white with dark key/output colours; Amber keeps roles rather than importing unrelated neon colours.

## Remaining work / release gate

The following requests are not declared complete:

1. **Target CI results** and pluginval 5 must be green on Mac and Win. CMake/workflow changes are implemented; do not confuse configuration with a successful run.
2. **Pro Tools/AAX, Reaper and Live manual tests**, real monitor/DPI changes and native GL open/close stress. The 100 editor test is component lifecycle without a native host peer.
3. **Native A/B profiling:** native/software/GL, idle vs signal, 1/8/32 instances, 100 peer attach/detach cycles. No CPU/GPU improvement numbers are available; GL remains off.
4. **TSan across UI/audio**, full GUI ASan/UBSan and target leak checks. A concurrency test exists; it does not prove absence of races without instrumentation.
5. **Visual refinements:** the optional GPU renderer does not yet share the accumulated software phosphor buffer; its blur needs native QA. Age brightness fade is implemented, while age-dependent sharpness is approximated by the low-resolution trail. Material shadows are cached blurred paths; inset glass depth uses the cached directional fill/grid rather than a separate ambient-occlusion simulation. The NOW cue detects a real rising reduction edge and does not claim to expose the DSP onset detector directly.
6. **Hardware null renders** on musical session materials, not only portable deterministic signals. Invalid-buffer bug fixes are intentionally outside the bit-exact promise.

Archive notes and `FILES-TO-DELETE.txt` were not removed. Proposed cleanup: move historic notes/tests to `Docs/Archive` once approved; retain the baseline headers and active regression suite. Reconcile duplicated old workflows after checking which is actually active. Production AAX licensing/PACE signing is a separate release requirement; CI developer AAX packaging does not establish it.

## Target-test adjustment

The first Mac TSan run passed the five engine regressions and stopped on the processor impulse assertion's exact float equality; it did not report a data race. JUCE's float parameter conversion/snapping may contract multiply-add on ARM (for example, nominal 0 dB can become approximately ±3e-7 dB). The processor impulse test now checks level within 1e-6 while retaining the exact expected sample position and detailed failure diagnostics. The frozen-baseline DSP null and 100%-AUTO comparisons remain bit-exact; DSP/parameter ranges were not changed to satisfy the test. The next target run determines whether this resolves the assertion.
