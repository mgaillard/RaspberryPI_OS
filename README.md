# RaspberryPI_OS

## Getting started

### Prerequisites

On Ubuntu 22.04 or 24.04, install the following packages:
```bash
$ sudo apt install gcc-arm-none-eabi gdb-multiarch qemu-system clang-format
```

### Build
```bash
$ make
```

### Run in QEMU with gdb
```bash
$ cd tools
$ ./run-qemu.sh
# Open another terminal and launch
$ ./run-gdb.sh
```

### Run in QEMU with VS Code Debugger

Make sure you have on of the following extensions installed:
- Native Debug
- C/C++ Extension Pack


Run QEMU in the terminal
```bash
$ cd tools
$ ./run-qemu.sh
```

Then launch the debugger in VS Code with one of the following configurations
- Attach to QEMU (Native)
- Attach to QEMU (VS Code)
