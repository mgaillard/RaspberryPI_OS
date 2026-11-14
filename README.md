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


### Run on a Raspberry Pi Model B (bare metal)

This kernel targets the first-generation Raspberry Pi Model B (ARM1176/BCM2835).
It is not currently configured for the Raspberry Pi 2 Model B or later models.

1. Build the SD-card kernel image:
	```bash
	$ make kernel_for_sdcard
	```
	This creates `build/kernel.img`.
2. Prepare a microSD card with a FAT32 boot partition and the Raspberry Pi boot
	firmware files, such as `bootcode.bin`, `start.elf`, and `fixup.dat`. You can
	use the boot partition from a Raspberry Pi OS image for the first-generation
	Model B; Linux itself is not needed to run this kernel.
3. Edit `config.txt` on the boot partition (or create it if missing). Set the
	following options under `[all]` so they apply to this board:
	```ini
	[all]
	kernel=my-kernel.img
	arm_64bit=0
	auto_initramfs=0
	enable_uart=1
	init_uart_clock=3000000
	```
	If `auto_initramfs=0` is already set before any section header, it applies
	globally and does not need to be repeated.
4. Copy `build/kernel.img` to the root of the boot partition as `my-kernel.img`.
	For a card mounted at `/run/media/mgaillard/bootfs`:
	```bash
	$ cp build/kernel.img /run/media/mgaillard/bootfs/my-kernel.img
	$ sync
	$ cmp build/kernel.img /run/media/mgaillard/bootfs/my-kernel.img
	```
	Replace the mount path if yours differs. Using `my-kernel.img` preserves the
	existing OS kernel. After the copy succeeds and `cmp` exits successfully
	without output, safely eject the card, insert it into the powered-off Pi,
	and power it on.

For serial output, connect a 3.3 V USB-to-TTL adapter to GPIO14/TXD (physical pin
8), GPIO15/RXD (pin 10), and ground (pin 6). Do not connect the adapter's 5 V
pin. The QEMU setup below does not exercise the Pi's SD-card boot firmware.
