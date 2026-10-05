# Pro Tools macOS performance validation

## Windows input responsiveness

The Windows build backports official JUCE commit
`d3d7d4c89c5547c09ff28d7ab13d9f3bccc8595e` from 8.0.5 into 8.0.4.
It skips missed VBlank events instead of immediately reposting a callback after
an overrunning frame. CMake checks the original normalized source fingerprint;
macOS is not patched. The vendored file retains JUCE's copyright/license notice.

Static chrome is still generated in software, but Windows Direct2D now keeps
one native image per chrome revision instead of converting/copying the full
software image on every paint. Resize, DPI and theme changes refresh that copy.
Software rendering continues to use the software cache directly.

The native Windows smoke test checks four editors, native cache reuse, and two
seconds of real-time audio on a worker thread for every available renderer.
All four peers switch renderer together. A native 20 ms WM_TIMER heartbeat
measures low-priority message dispatch gaps; a gap of one second fails the
test. This is a starvation regression test, not a claim of 60 FPS or measured
mouse-input latency in Ableton. Full-component snapshot timings are not live
frame timings.

On a Windows PC, test Ableton Live with 1/4/8 open VST3 editors for at least
30 seconds of playback at 100% and 200% display scaling. Test every renderer
with ALL windows switched (or close/reopen them after saving the preference).
Drag Influence and Attack, click freeze, move windows, and stop/resume playback.
Measure message-thread CPU and input delay separately from visual frame rate.
The target remains 60 FPS for short history windows and 30 FPS for long ones;
overload must drop stale frames rather than starve input. Include Ableton
Spectrum as another source of graphics load. Repeat with graphs frozen to
separate graph preparation/rasterisation from audio processing.

## macOS host checks

This repository's CI builds a universal AAX and checks its signature and both
architectures. It cannot measure Pro Tools' editor or audio behaviour. Run the
following on an Apple silicon Mac with Pro Tools, using the same session and
hardware buffer for each case.

1. Insert one Duck Pocket AAX on a playing stereo track. Record the Pro Tools
   version, macOS version, native/Rosetta mode, sample rate, display scaling,
   session buffer size and plug-in build number.
2. Observe playback for 30 seconds with the plug-in editor closed. Repeat with
   the editor open at 100 ms, 1 s, 2 s and 5 s. Use Solid Dark first; repeat
   100 ms and 5 s with Neon. Repeat 5 s with three open instances.
3. Record GUI responsiveness, Pro Tools CPU/AAE meter, any audio underruns,
   and frame rate (a screen recording is sufficient for visible stutters).
4. Attach Instruments **Time Profiler** to Pro Tools during each 30-second run.
   Inspect the main-thread time under `DuckPocketAudioProcessorEditor::graph`,
   `juce::PathStrokeType::createStrokedPath`, path flattening, and image drawing.
   Inspect the audio thread separately for `processAudio`/`pocket::Engine::process`.
5. Compare against the previous release and the VST3 in a macOS host with the
   same window and themes. If GUI time improves but audio underruns remain with
   the editor closed, investigate the audio path as a separate fault.

Acceptance: the 5 s AAX window should play without new underruns and should
keep the host responsive during a 30-second run, including with three editors
open. Check that peaks and gain minima remain visible and that freeze/resume,
resize, theme switches and presets behave correctly for all window sizes.

The 2 ms rollup only affects display data at 2 s and 5 s. DSP audio and saved
active parameter values are unchanged by the graph optimisation.
