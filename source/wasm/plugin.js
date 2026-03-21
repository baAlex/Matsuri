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
export const MIDI_CLOSED_HIT_HAT_KEY = 42;
export const MIDI_OPEN_HIT_HAT_KEY = 46;


let s_wasm_array = null;


export async function FetchAndRegister(audio_context) {

	// Fetch worklet code, register it on the context
	await audio_context.audioWorklet.addModule("./matsuri-worklet.js");

	// Fetch WASM blob, make it an array
	if (s_wasm_array == null) {
		const response = await fetch("./matsuri.wasm");
		s_wasm_array = await response.arrayBuffer();
	}
}

export class MatsuriNode extends AudioWorkletNode {

	constructor(audio_context) {
		super(audio_context, "matsuri-processor");

		// Send WASM to worklet so it can finish its initialisation there
		this.port.postMessage({ type: "initialise", array: s_wasm_array });
	}

	keyOn(no) {
		this.port.postMessage({ type: "key-on", key_no: no });
	}
}
