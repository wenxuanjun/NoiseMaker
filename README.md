# Noise Maker

A small tool to play music through the PC speaker in Linux

## Usage

You need to build it first

```bash
g++ sound.cpp -o sound.o
```

Run it with root permission

```bash
sudo ./sound.o <data.txt>
```

Or you can run this command in order to run commands above automatically

```bash
./run.sh <data.txt>
```
