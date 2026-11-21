# Baggy Delay (VST3)

Stereo delay z interfejsem ze stickmanem w ultra szerokich jeansach. Im mocniej kręcisz gałki,
tym bardziej czerwone ma oczy, niżej powieki, więcej dymu i czerwieńsze tło.

Parametry: TIME (1-2000 ms), FEEDBACK, TONE (lowpass w feedbacku), WIDTH (ping-pong), MIX.

## Budowanie (Windows)

Wymagane: Visual Studio 2022 (Desktop C++), CMake 3.22+, Git.

    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Release

CMake sam pobierze JUCE. Po buildzie plugin skopiuje się do `C:\Program Files\Common Files\VST3`
(odpal terminal jako administrator, albo skopiuj `Baggy Delay.vst3` z folderu `build` ręcznie).
Potem w FL Studio: Options -> Manage plugins -> Find installed plugins.
