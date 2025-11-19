# Moto: Modern Bootloader for Quansheng UV-K5 (V3) and Variants

Moto, a modern bootloader for the UV-K5 (V3) and its variants (UV-K6, UV-K1, etc.), takes its name from the Japanese word "もと / 元," meaning "origin."

It mounts the radio as a USB disk on a computer, providing a drag-and-drop user experience for flashing firmware or data and creating backups.

Features: 

- Compliant with the [UF2](https://github.com/microsoft/uf2) bootloader specification
- Mounts the radio as a USB disk to provide a drag-and-drop user experience
- Replace the stock bootloader without requiring any firmware modifications
- The same simple drag-and-drop method for upgrading or restoring the stock bootloader
- Firmware readout and flashing
- Data storage (SPI flash) read and write
- FAT file system
- UF2 firmware file format
- :x: No support for serial commands

## Usage

> [!CAUTION]
> Although Moto makes updating firmware and data very simple, it is a powerful tool and careless or improper handling can lead to catastrophic consequences, such as data corruption including calibration data. This data is unique to each device and, once lost, is almost impossible to recover. No one other than the user themselves — including the author of Moto — shall be held responsible for any consequences.

Moto will replace the original bootloader, so special tools are required to flash it to the radio. It is recommended to use [Ichi](https://github.com/muzkr/ichi), a sister project specifically designed for updating bootloaders.

> [!WARNING]
> Updating the bootloader is a risky operation. An incomplete update may render the device unable to boot again. No one other than the user themselves — including the author of Moto or Ichi — shall be held responsible for any consequences.

Just like with the stock bootloader, holding down the PTT button while powering on the device will enter Moto's DFU mode. 

Connect the device to your computer using a USB Type-C cable. A disk drive labeled "MOTO" will appear on your computer. This disk represents the radio's internal storage space. Inside the MOTO disk, you will find:

- CURRENT.UF2: Current firmware
- DATA.UF2: 2 MB of data storage

You can copy these files to a safe place as backups.

Moto supports writing (flashing) firmware or data in the UF2 format. To update the firmware or SPI flash, prepare a UF2 file, copy it to the MOTO disk, and Moto will then write the data to the internal or SPI flash based on the UF2 metadata. 

For detailed instructions, please refer to the [wiki](https://github.com/muzkr/moto/wiki).

## License

Apache 2.0

Third-party components and copyright information can be found in the 3rd_party_licenses.txt file.
