# Moto: Modern Bootloader for Quansheng UV-K5 (V3) and UV-K1 Radios

Moto (Japanese "もと / 元", meaning "origin") is a modern bootloader that mounts the radio as a USB disk on a computer, providing a drag-and-drop user experience for firmware flashing and backups.

Features: 

- Radio as USB disk to provide a drag-and-drop user experience
- FAT file system
- UF2 firmware file format
- :x: No serial commands support

## Usage

Moto will replace the original bootloader, so special tools are required to flash it to the radio. It is recommended to use [Ichi](https://github.com/muzkr/ichi), a sister project of this one, which is specifically designed for updating bootloaders.

Just like the stock bootloader, holding down the PTT button while powering on will enter Moto's DFU mode. 

Connect the device to your computer using a USB Type-C cable. A disk drive labeled "MOTO" will appear on your computer. This disk represents the internal storage space of the radio. The CURRENT.UF2 file contains the current firmware and can be copied elsewhere as a backup.

Copy the firmware file (in UF2 format) that needs to be updated to the device to the MOTO disk to perform the flashing. Typically, the process completes in seconds, after which the device will automatically reboot and start up with the updated firmware.


## License

Apache 2.0

Third-party components and copyright information can be found in the 3rd_party_licenses.txt file.
