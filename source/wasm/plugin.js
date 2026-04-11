/*

MIT No Attribution

Copyright 2026 Alexander Brandt

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

export const MIDI_ACOUSTIC_BASS_DRUM_KEY = 35;
export const MIDI_BASS_DRUM_KEY = 36;
export const MIDI_ACOUSTIC_SNARE_KEY = 38;
export const MIDI_ELECTRIC_SNARE_KEY = 40;

export const MIDI_LOW_FLOOR_TOM_KEY = 41;
export const MIDI_HIGH_FLOOR_TOM_KEY = 43;
export const MIDI_LOW_TOM_KEY = 45;

export const MIDI_LOW_MID_TOM_KEY = 47;
export const MIDI_HIGH_MID_TOM_KEY = 48;
export const MIDI_HIGH_TOM_KEY = 50;

export const MIDI_CLOSED_HIT_HAT_KEY = 42;
export const MIDI_OPEN_HIT_HAT_KEY = 46;
export const MIDI_CRASH_CYMBAL_KEY = 49;


let s_wasm_array = null;


export async function FetchAndRegister(audio_context) {

	// Fetch worklet code, register it on the context
	await audio_context.audioWorklet.addModule("./matsuri-v2-worklet.js");

	// Fetch WASM blob, make it an array
	if (s_wasm_array == null) {
		const response = await fetch("./matsuri-v2.wasm");
		s_wasm_array = await response.arrayBuffer();
	}
}

export class MatsuriV2Node extends AudioWorkletNode {

	constructor(audio_context) {
		super(audio_context, "matsuri-v2-processor");

		// Send WASM to worklet so it can finish its initialisation there
		this.port.postMessage({ type: "Initialise", array: s_wasm_array });

		this.port.onmessage = (event) => {
			console.log(event.data);
		};
	}

	noteOn(note_no, velocity = 0.5) {
		// 144 = Note On, Channel 0
		const v = Math.max(Math.min(velocity, 1) * 127, 1);
		this.port.postMessage({ type: "Midi", byte0: 144, byte1: note_no, byte2: v });
	}

	midi(byte0, byte1, byte2) {
		this.port.postMessage({ type: "Midi", byte0, byte1, byte2 });
	}
}
