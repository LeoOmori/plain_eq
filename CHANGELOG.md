# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-06-16

### Added

- Multi-band parametric EQ plugin built with JUCE.
- Peaking, low-shelf, high-shelf, low-pass, and high-pass filter types.
- Frequency, gain, Q, output gain, and bypass controls.
- Drag-and-edit EQ nodes on the graph.
- Per-node filter type selection from the UI.
- Mouse wheel Q adjustment for the selected node.
- Live input spectrum analyzer using `juce::dsp::FFT`.
- Standalone app, VST3, and AU plugin targets.

[1.0.0]: https://github.com/LeoOmori/plain_eq/releases/tag/v1.0.0
