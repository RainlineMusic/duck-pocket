# Logo quack

Source: duck.wav supplied by the project owner for this logo interaction. The original stereo 24-bit / 48 kHz recording is averaged to mono, with 5 ms endpoint fades, then polyphase resampled offline (SciPy resample_poly) to the six labelled rates and stored as 16-bit PCM. No normalization or amplification. Duration approximately 0.508 s. Only the chosen rate is decoded during prepareToPlay; playback never resamples or opens a file.
