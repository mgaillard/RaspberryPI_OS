# Back up and restore an SD card

These instructions create a full-card image on Linux, preserving the partition
table, boot files, operating system, and data. Store the image on your computer,
not on the SD card. You need free space equal to the card's entire capacity,
even if its files use less space.

## Identify and unmount the card

Power off the Raspberry Pi before removing its SD card, then connect the card
to your computer.

```bash
lsblk -o NAME,PATH,SIZE,MODEL,TYPE,FSTYPE,MOUNTPOINTS
```

Identify the card by its size and partitions. The examples below assume the
whole card is `/dev/mmcblk0`, with partitions `/dev/mmcblk0p1` and
`/dev/mmcblk0p2`. Substitute your actual device names if different. Recheck
after every reconnection: device names can change.

Unmount every mounted partition on the card before backing up or restoring.
For example, if both partitions are mounted:

```bash
sudo umount /dev/mmcblk0p1 /dev/mmcblk0p2
```

The command is `umount`, not `unmount`. If a partition is already unmounted,
omit it. If a partition is busy, close file manager windows and terminals using
it, then retry. Do not proceed while any card partition remains mounted.

Run `lsblk` again and check that the card's partitions have empty `MOUNTPOINTS`.
Leave them unmounted throughout copying and verification.

## Create the backup

Check available space on the filesystem containing your home directory:

```bash
df -h "$HOME"
```

Copy the entire card, not an individual partition:

```bash
sudo dd if=/dev/mmcblk0 of="$HOME/raspberry-pi-backup.img" \
  bs=4M status=progress conv=fsync,excl
```

- `if` is the input device; `of` is the output image file.
- `bs=4M` copies in 4 MiB blocks; `status=progress` displays progress.
- `conv=fsync` flushes the output before completion.
- `conv=excl` refuses to overwrite an existing backup. It belongs in `conv`,
  not `oflag`.

Wait for `dd` to finish successfully. If the file already exists, choose another
backup filename rather than overwriting it. If copying fails, do not treat the
partial image as a valid backup.

## Verify the backup

Before overwriting the card, compare the image against the original card:

```bash
sudo cmp "$HOME/raspberry-pi-backup.img" /dev/mmcblk0 &&
  echo "Backup verified"
```

`cmp` reads both inputs and compares them byte for byte. It produces no output
on success; the confirmation appears only when the comparison succeeds.
Any difference or read error means verification failed. Do not overwrite the
original card until verification succeeds.

Keep the verified image in a safe location. It may contain passwords, keys, and
other private files from the original OS.

## Restore the image to an SD card

**Warning: restoration overwrites the destination card's partition table and
data. Check the device carefully; selecting your computer's disk can destroy
its contents.**

Reconnect the destination card, identify it with `lsblk`, and unmount all its
mounted partitions as described above. The destination must be at least as
large as the image in bytes; cards with the same advertised capacity can differ.

Check the image size and destination capacity:

```bash
stat -c '%s bytes' "$HOME/raspberry-pi-backup.img"
lsblk -b -o NAME,PATH,SIZE,TYPE /dev/mmcblk0
```

After confirming the destination device and capacity, write the image back:

```bash
sudo dd if="$HOME/raspberry-pi-backup.img" of=/dev/mmcblk0 \
  bs=4M status=progress conv=fsync
```

Here the input and output are reversed: the image is read and the whole card
is overwritten. Do not use `excl` when restoring to an existing device.
Wait for `dd` to finish successfully.

Verify the restored bytes:

```bash
sudo cmp -n "$(stat -c %s "$HOME/raspberry-pi-backup.img")" \
  "$HOME/raspberry-pi-backup.img" /dev/mmcblk0 &&
  echo "Restore verified"
```

The byte limit allows verification on a destination card larger than the image.
Restoration preserves the original partition sizes; it does not automatically
expand them to fill a larger card.

After successful verification, safely eject the card using your desktop's disk
or file manager before removing it and inserting it into the powered-off Pi.
