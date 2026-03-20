
// https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletProcessor#examples
// https://jamiebeverley.net/ramblings/RustWASMAudioWorklets.html

class MatsuriProcessor extends AudioWorkletProcessor {
	constructor() {
		super();

		this.m_wasm = null;
		this.m_view = null;
		this.early_events = [];

		this.port.onmessage = async (event) => {
			// Initialise
			if (event.data.type == "initialise") {

				// Bureaucracy
				const obj = await WebAssembly.instantiate(event.data.wasm_array);
				this.m_wasm = obj.instance.exports;

				const pointer = this.m_wasm.Initialise(sampleRate);
				this.m_view = new Float32Array(this.m_wasm.memory.buffer, pointer, 128);

				// Queued events?
				for (const q of this.early_events) {
					if (q.type == "key-on")
						this.m_wasm.KeyOn(q.key_no);
				}

				this.early_events.length = 0;
			}

			// Key On
			else if (event.data.type == "key-on") {
				if (this.m_wasm == null) // Ouch...
					this.early_events.push(event.data);
				else
					this.m_wasm.KeyOn(event.data.key_no);
			}
		};
	}

	process(inputs, outputs, parameters) {
		if (this.m_wasm == null || this.m_view == null)
			return true;

		this.m_wasm.Render(this.m_view.length);

		const out = outputs[0];
		for (let ch = 0; ch < out.length; ch++)
			out[ch].set(this.m_view);

		return true;
	}
}

registerProcessor("matsuri-processor", MatsuriProcessor);
