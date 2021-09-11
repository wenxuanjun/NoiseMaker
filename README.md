# Noise Maker

A small tool to play music through the PC speaker in Linux

## Usage

You need to build it first

```bash
g++ sound.cpp -o sound.o
```

Edit `data.txt` which contains the data of the music

```bash
vim data.txt # Use your favorite editor
```

When it is all done, run it with root permission

```bash
sudo ./sound.o
```

Or you can run this command in order to run commands above automatically

```bash
./run.sh
```
