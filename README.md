
Matsuri
=======

Synthesizer recreating the sound of Roland's TR-606 drum machine. Fast, tiny, and quite close to the original sounds (considering it doesn't emulate circuitry). The project includes:

- CLAP plugin (work in progress)
- Web Audio / WebAssembly [AudioNode](https://developer.mozilla.org/en-US/docs/Web/API/AudioNode)
- Samples with an SFZ definition

[*] This project has no relation with Roland, all trademarks and copyrights are theirs.


Download
--------
**Soon**. But in the meantime you can [compile the code](https://github.com/baAlex/Matsuri?tab=readme-ov-file#clone-and-compile-code).


Tested platforms and DAWs
-------------------------
- CLAP plugin
	- [Zrythm](https://www.zrythm.org/en/index.html), on Linux
	- [Bass Studio](https://bass-studio.com/), in Wine on Linux
- Web Audio / WebAssembly AudioNode
	- Firefox 148
	- Chrome 145


Clone and compile code
----------------------
With `git`, `cmake`, and a C11 compiler installed, cloning and compilation should be:

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

Files created from code/programs here are under no license, those are yours.
