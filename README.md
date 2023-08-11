# Noise Maker

A small tool to play music through the PC speaker in Linux

## Usage

You need to build it first:

```bash
g++ sound.cpp -o sound.o
```

Then run it with **root** permission:

```bash
sudo ./sound.o <data.txt>
```

Some examples are provided here:

```bash
sudo ./sound.o examples/bears.txt
```