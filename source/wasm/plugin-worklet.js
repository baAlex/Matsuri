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

class MatsuriV2Processor extends AudioWorkletProcessor {

	// https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletNode/parameters

	static get parameterDescriptors() {
		// https://developer.mozilla.org/en-US/docs/Web/API/AudioParam#k-rate
		return [
			{ name: "gain", defaultValue: 1.0, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "gain-bass-drum", defaultValue: 1.0, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "gain-snare-drum", defaultValue: 1.0, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "gain-closed-hit-hat", defaultValue: 0.2, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "gain-open-hit-hat", defaultValue: 0.2, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "gain-cymbal", defaultValue: 0.5, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "gain-low-tom", defaultValue: 0.75, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "gain-high-tom", defaultValue: 0.75, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "velocity-gain-modulation", defaultValue: 1.0, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "velocity-tone-modulation", defaultValue: 1.0, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
			{ name: "reference-velocity", defaultValue: 0.5, minValue: 0.0, maxValue: 1.0, automationRate: "k-rate" },
		];
	}

	// https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletProcessor#examples
	// https://jamiebeverley.net/ramblings/RustWASMAudioWorklets.html

	constructor() {
		super();

		this.m_wasm = null;
		this.m_view = null;
		this.early_events = [];

		this.port.onmessage = async (event) => {
			// Initialise
			if (event.data.type == "Initialise") {

				// Bureaucracy
				const obj = await WebAssembly.instantiate(event.data.array);
				this.m_wasm = obj.instance.exports;

				const pointer = this.m_wasm.Initialise(sampleRate);
				this.m_view = new Float32Array(this.m_wasm.memory.buffer, pointer, 128);

				// Queued events?
				for (const q of this.early_events) {
					if (q.type == "Midi")
						this.m_wasm.Midi(q.byte0, q.byte1, q.byte2);
				}

				this.early_events.length = 0;
			}

			// Midi
			else if (event.data.type == "Midi") {
				if (this.m_wasm == null)
					this.early_events.push(event.data);
				else
					this.m_wasm.Midi(event.data.byte0, event.data.byte1, event.data.byte2);
			}
		};
	}

	process(inputs, outputs, parameters) {
		if (this.m_wasm == null || this.m_view == null)
			return true;

		this.m_wasm.Render(
			parameters["gain"][0],
			parameters["gain-bass-drum"][0],
			parameters["gain-snare-drum"][0],
			parameters["gain-closed-hit-hat"][0],
			parameters["gain-open-hit-hat"][0],
			parameters["gain-cymbal"][0],
			parameters["gain-low-tom"][0],
			parameters["gain-high-tom"][0],
			parameters["velocity-gain-modulation"][0],
			parameters["velocity-tone-modulation"][0],
			parameters["reference-velocity"][0],
			this.m_view.length);

		const out = outputs[0];
		for (let ch = 0; ch < out.length; ch += 1)
			out[ch].set(this.m_view);

		return true;
	}
}

registerProcessor("matsuri-v2-processor", MatsuriV2Processor);
