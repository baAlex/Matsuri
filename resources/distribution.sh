#!/bin/bash

./generate-wavs

ffmpeg -i 606-bass-drum.wav -y 606-bass-drum.flac
ffmpeg -i 606-bass-drum-mid.wav -y 606-bass-drum-mid.flac
ffmpeg -i 606-bass-drum-max.wav -y 606-bass-drum-max.flac

ffmpeg -i 606-snare-drum.wav -y 606-snare-drum.flac
ffmpeg -i 606-snare-drum-mid.wav -y 606-snare-drum-mid.flac
ffmpeg -i 606-snare-drum-max.wav -y 606-snare-drum-max.flac

ffmpeg -i 606-hat-closed.wav -y 606-hat-closed.flac
ffmpeg -i 606-hat-closed-mid.wav -y 606-hat-closed-mid.flac
ffmpeg -i 606-hat-closed-max.wav -y 606-hat-closed-max.flac

ffmpeg -i 606-hat-open.wav -y 606-hat-open.flac
ffmpeg -i 606-hat-open-mid.wav -y 606-hat-open-mid.flac
ffmpeg -i 606-hat-open-max.wav -y 606-hat-open-max.flac

ffmpeg -i 606-cymbal.wav -y 606-cymbal.flac
ffmpeg -i 606-cymbal-mid.wav -y 606-cymbal-mid.flac
ffmpeg -i 606-cymbal-max.wav -y 606-cymbal-max.flac

ffmpeg -i 606-tom-low.wav -y 606-tom-low.flac
ffmpeg -i 606-tom-low-mid.wav -y 606-tom-low-mid.flac
ffmpeg -i 606-tom-low-max.wav -y 606-tom-low-max.flac

ffmpeg -i 606-tom-high.wav -y 606-tom-high.flac
ffmpeg -i 606-tom-high-mid.wav -y 606-tom-high-mid.flac
ffmpeg -i 606-tom-high-max.wav -y 606-tom-high-max.flac

cp -f ../resources/matsuri-606.sfz matsuri-606.sfz

zip -9 -D matsuri.zip \
606-bass-drum.flac 606-bass-drum-mid.flac 606-bass-drum-max.flac \
606-snare-drum.flac 606-snare-drum-mid.flac 606-snare-drum-max.flac \
606-hat-closed.flac 606-hat-closed-mid.flac 606-hat-closed-max.flac \
606-hat-open.flac 606-hat-open-mid.flac 606-hat-open-max.flac \
606-cymbal.flac 606-cymbal-mid.flac 606-cymbal-max.flac \
606-tom-low.flac 606-tom-low-mid.flac 606-tom-low-max.flac \
606-tom-high.flac 606-tom-high-mid.flac 606-tom-high-max.flac \
matsuri-606.sfz
