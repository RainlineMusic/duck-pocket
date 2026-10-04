# Designer artwork

`Reference.svg` is the SVG supplied by the user for this project. Derived curved headings and logo preserve its vector paths. `DialLarge.png` and `DialSmall.png` are transparent 4× material layers, exported with Inkscape so SVG blur/inner-shadow filters retain their appearance; interactive rings, markers and readouts are excluded. Reproduce them with `python3 Tools/extract_designer_assets.py` (Inkscape required). These exported assets are committed, so a plugin build does not require Inkscape.

Inter Regular/Medium come from https://github.com/rsms/inter (`docs/font-files/Inter-Regular.woff2` and `Inter-Medium.woff2`), converted losslessly to TTF containers for JUCE. Their upstream OFL is in `Assets/Fonts/Inter-OFL.txt` and copied into delivered plugin bundles.
