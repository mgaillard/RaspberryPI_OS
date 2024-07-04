# RaspberryPI_OS

## Getting started

### Prerequisites

On Ubuntu 22.04 or 24.04, install the following packages:
```bash
$ sudo apt install gcc-arm-none-eabi gdb-arm-none-eabi qemu-system
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
