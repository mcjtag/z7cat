# z7cat
Zynq-7000 MCS Creator.

App creates .mcs file to program zynq-7000 flash memory.

## Build
To build app type command: $ make

To clean type command: $ make clean

## Usage example

Possible partition table:

|  dev  |    addr    |     size    |    name   |
|-------|------------|-------------|-----------|
| mtd0: | 0x00000000 |  0x00500000 |  "boot"   |
| mtd1: | 0x00500000 |  0x00020000 | "bootenv" |
| mtd2: | 0x00520000 |  0x00a80000 | "kernel"  |
| mtd3: | 0x00fa0000 |  0x00000000 |  "spare"  |

We have two files: BOOT.BIN and image.ub. BOOT.BIN contains: fsbl, bitstream and u-boot. image.ub contains: kernel and rootfs content.

To create mcs file from these file we need to type command:
> z7cat path_to/BOOT.BIN 0x0  path_to/image.ub 0x520000 -o path_to/image.mcs 0x1000000

Thus, BOOT.BIN starts from 0x00000000 address and image.ub starts from 0x00520000 address.

image.mcs is output file name and '0x1000000' is output binary size (memory flash size, in this case 16 Mbytes).

## Cautions
App does not support collision checking.
