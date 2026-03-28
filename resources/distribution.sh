#!/bin/bash

./matsuri-v2-wavs

ffmpeg -i 606-bass-drum.wav -y 606-v2-bass-drum.flac
ffmpeg -i 606-bass-drum-mid.wav -y 606-v2-bass-drum-mid.flac
ffmpeg -i 606-bass-drum-max.wav -y 606-v2-bass-drum-max.flac

ffmpeg -i 606-snare-drum.wav -y 606-v2-snare-drum.flac
ffmpeg -i 606-snare-drum-mid.wav -y 606-v2-snare-drum-mid.flac
ffmpeg -i 606-snare-drum-max.wav -y 606-v2-snare-drum-max.flac

ffmpeg -i 606-hat-closed.wav -y 606-v2-hat-closed.flac
ffmpeg -i 606-hat-closed-mid.wav -y 606-v2-hat-closed-mid.flac
ffmpeg -i 606-hat-closed-max.wav -y 606-v2-hat-closed-max.flac

ffmpeg -i 606-hat-open.wav -y 606-v2-hat-open.flac
ffmpeg -i 606-hat-open-mid.wav -y 606-v2-hat-open-mid.flac
ffmpeg -i 606-hat-open-max.wav -y 606-v2-hat-open-max.flac

ffmpeg -i 606-cymbal.wav -y 606-v2-cymbal.flac
ffmpeg -i 606-cymbal-mid.wav -y 606-v2-cymbal-mid.flac
ffmpeg -i 606-cymbal-max.wav -y 606-v2-cymbal-max.flac

ffmpeg -i 606-tom-low.wav -y 606-v2-tom-low.flac
ffmpeg -i 606-tom-low-mid.wav -y 606-v2-tom-low-mid.flac
ffmpeg -i 606-tom-low-max.wav -y 606-v2-tom-low-max.flac

ffmpeg -i 606-tom-high.wav -y 606-v2-tom-high.flac
ffmpeg -i 606-tom-high-mid.wav -y 606-v2-tom-high-mid.flac
ffmpeg -i 606-tom-high-max.wav -y 606-v2-tom-high-max.flac

cp -f ../resources/matsuri-v2-606.sfz matsuri-v2-606.sfz

zip -9 -D matsuri-v2.zip \
606-v2-bass-drum.flac 606-v2-bass-drum-mid.flac 606-v2-bass-drum-max.flac \
606-v2-snare-drum.flac 606-v2-snare-drum-mid.flac 606-v2-snare-drum-max.flac \
606-v2-hat-closed.flac 606-v2-hat-closed-mid.flac 606-v2-hat-closed-max.flac \
606-v2-hat-open.flac 606-v2-hat-open-mid.flac 606-v2-hat-open-max.flac \
606-v2-cymbal.flac 606-v2-cymbal-mid.flac 606-v2-cymbal-max.flac \
606-v2-tom-low.flac 606-v2-tom-low-mid.flac 606-v2-tom-low-max.flac \
606-v2-tom-high.flac 606-v2-tom-high-mid.flac 606-v2-tom-high-max.flac \
matsuri-v2-606.sfz \
matsuri-v2-wasm/matsuri-v2.wasm matsuri-v2-wasm/matsuri-v2.js matsuri-v2-wasm/matsuri-v2-worklet.js \
matsuri-v2.clap
