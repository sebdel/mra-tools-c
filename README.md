# mra-tools-c
![C/C++ CI](https://github.com/sebdel/mra-tools-c/workflows/C/C++%20CI/badge.svg)

A tool used to merge mame ROMs in various ways, using the so called MRA configuration file.
Also useful to generate all sort of binary blobs.

## Download
Pre-compiled binaries for linux and windows 64bits are available under the release folder.

## How to build
```bash
git clone https:\\https://github.com/sebdel/mra-tools-c.git
make
```

### Build for windows
```bash
sudo apt install mingw-w64 libz-mingw-w64-dev
make -f makefile.windows
```

### Build using Docker

In some platforms such as MacOS the tool needs to be used inside a linux container. Just clone the repo:
```bash
git clone https:\\https://github.com/sebdel/mra-tools-c.git
```

Copy all MRA files and zipped files in two directories and then:

```bash
cd mra-tools-c
ROMS=/my/roms/directory  MRAS=/my/mra_files/directory docker-compose up --build mra
```

The .rom and .arc files will be generated inside your mra_files directory.

## License

You do whatever you want with it, it does whatever it wants to your PC. Don't credit me, don't blame me :)
