
Matsuri
=======

Synthesizer recreating the sound of Roland's TR-606 drum machine. Fast, tiny, and close to original sounds (considering it doesn't emulate circuitry). The project includes:

- CLAP plugin
	- With velocity affecting both volume and timbre (configurable), optional limiter, volume controls, choking between hats, and each sound being unique.
- Web Audio / WebAssembly [AudioNode](https://developer.mozilla.org/en-US/docs/Web/API/AudioNode)
	- Same capabilities as CLAP plugin.
- FLAC samples with an SFZ definition
	- For old-school musicians, they are more rigid: 4 velocities only. SFZ definition implements volume controls, and choking.


[*] This project has no relation with Roland, all trademarks and copyrights are theirs.


Download
--------
**Soon**. But in the meantime you can [compile the code](#clone-and-compile-code).


Guide for thr Web Audio node
----------------------------

For now there's no "automated deployment" of any kind, you have two routes:
- **Download latest release** (I recommend this), there you will find a precompiled WASM module with its JS companions.
- Compile the WASM module manually, with `clang`, and `cmake` ([see below](#clone-and-compile-code)).

Regardless of which method you choose, your will find a folder containing following stuff:

- `matsuri-v2.wasm`, WASM module, there's not much you can do with it.
- `matsuri-v2-worklet.js`, glue between JS and WASM, it's a boring file, don't touch it.
- `matsuri-v2.js`, **this is the important one**, what may be called the library/module. It defines `MatsuriV2Node` class (a Web Audio node), and it's in charge of fetching and initialise the worklet. **You absolutely should modify this file in order to better integrate it into your framework**, for example the fetching function `FetchAndRegister()` has a hardcoded line of code: `await fetch("./matsuri-v2.wasm")`, that you may want to change. This file is under public domain, so make it yours.
- `test-page-v2.html`, minimal example that showcase the synth, it's ugly but gets things done.

Got the files?, run this command on your terminal:
```
python3 -m http.server 8000 --directory <FOLDER CONTAINING ABOVE FILES>
```

Then go into your browser, usually address `http://0.0.0.0:8000/` (this information should be on the terminal). Choose the test page and everything should work, use the buttons on screen, or keys Z, X, N, M, G, J, L; or connect your MIDI keyboard.


### Code examples

Initialisation should be:
```js
import * as matsuri from "./matsuri-v2.js";

let ctx = new AudioContext();

// Tell the audio context what "Matsuri" is
await matsuri.FetchAndRegister(ctx);

// Create and connect a Matsuri node
let node = new matsuri.MatsuriV2Node(ctx);
node.connect(ctx.destination);

// Node will start playing, but in silence, as no note has been sent
```

To send a note:
```js
// Any of them (or all them, to test polyphony):
node.noteOn(matsuri.MIDI_BASS_DRUM_KEY, 0.5); // 0.5 = Velocity, use range [0, 1]
node.noteOn(matsuri.MIDI_ACOUSTIC_SNARE_KEY, 0.5);
node.noteOn(matsuri.MIDI_LOW_FLOOR_TOM_KEY, 0.5);
node.noteOn(matsuri.MIDI_LOW_MID_TOM_KEY, 0.5);
node.noteOn(matsuri.MIDI_CLOSED_HIT_HAT_KEY, 0.5);
node.noteOn(matsuri.MIDI_OPEN_HIT_HAT_KEY, 0.5);
node.noteOn(matsuri.MIDI_CRASH_CYMBAL_KEY, 0.5);
```

To adjust parameters it follows standard Web Audio API:
```js
let parameter = node.parameters.get("volume-bass-drum");
parameter.setValueAtTime(value, ctx.currentTime);
```

These are available parameters, with their default, minimum, and maximum values:

| Parameter name               | Default value | Minimum | Maximum |
| ---------------------------- | ------------- | ------- | ------- |
| `volume-bass-drum`           | 1             | 0       | 100     |
| `volume-snare-drum`          | 1             | 0       | 100     |
| `volume-closed-hit-hat`      | 0.65          | 0       | 100     |
| `volume-open-hit-hat`        | 0.7           | 0       | 100     |
| `volume-cymbal`              | 0.8           | 0       | 100     |
| `volume-low-tom`             | 1             | 0       | 100     |
| `volume-high-tom`            | 1             | 0       | 100     |
| `velocity-volume-modulation` | 1             | 0       | 1       |
| `velocity-tone-modulation`   | 1             | 0       | 1       |
| `velocity-reference`         | 0.5           | 0       | 1       |
| `limiter-decay`              | 0             | 0       | 1000    |
| `master-volume`              | 1             | 0       | 100     |


Clone and compile code
----------------------
With `git`, `cmake`, and a C11 compiler installed, cloning and compiling should be:

```
git clone --recurse-submodules https://github.com/baAlex/Matsuri
cd Matsuri
mkdir build
cd build
cmake ..
make
```


License
-------
Code under the MPL-2.0 license. Every file includes its respective notice.

Samples and audio created from code/programs, under no license, those are yours.
