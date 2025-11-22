# Moto: Modern Bootloader for Quansheng UV-K5 (V3) and Variants

Moto, a modern bootloader for the UV-K5 (V3) and its variants (UV-K6, UV-K1, etc.), takes its name from the Japanese word "もと / 元," meaning "origin."

It mounts the radio as a USB disk on a computer, providing a drag-and-drop user experience for firmware flashing or backup.

Features: 

- Compliant with the [UF2](https://github.com/microsoft/uf2) bootloader specification
- Mounts the radio as a USB disk to provide a drag-and-drop user experience
- Firmware backup and flashing
- Drag-and-drop upgrading, or restoring stock bootloader (via [Ichi](https://github.com/muzkr/ichi))
- Replaces stock firmware, compatible with all known firmware
- FAT file system
- UF2 firmware format
- :x: No support for serial commands

## Usage

Moto will replace the original bootloader, so special tools are required to flash it to the radio. It is recommended to use [Ichi](https://github.com/muzkr/ichi), a sister project specifically designed for updating bootloaders.

> [!WARNING]
> Updating the bootloader is a risky operation. An incomplete update may render the device unable to boot again. No one other than the user themselves shall be held responsible for any consequences.

Just like with the stock bootloader, holding down the PTT button while powering on the device will enter Moto's DFU mode. 

Connect the device to your computer using a USB Type-C cable. A disk drive labeled "MOTO" will appear on your computer. Inside the MOTO disk, you will find a CURRENT.UF2 file, which is the current firmware. You can copy it to a safe place as a backup.

To update (flash) firmware, simply copy the firmware (in UF2 format) to the MOTO disk, and Moto will do the rest. 

For detailed operating instructions, see also [doc/Basic-Operations.md](doc/Basic-Operations.md).

## License

Apache 2.0

Third-party components and copyright information can be found in the 3rd_party_licenses.txt file.
