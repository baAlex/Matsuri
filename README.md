
Matsuri
=======

Synthesizer recreating Roland's TR-606 drum machine¹. Fast, tiny (~10 kB), and quite close to original sounds (considering it doesn't emulate circuitry). The project includes, samples with a [SFZ definition](https://sfzformat.com/software/players/), and a Web Audio + WebAssembly [AudioNode](https://developer.mozilla.org/en-US/docs/Web/API/AudioNode). CLAP plugin in the works.

[1] This project has no relation with Roland, all trademarks and copyrights are theirs.


Clone and compile code
----------------------
With `git`, `cmake`, and a C99 compiler installed, cloning and compilation should be:

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
