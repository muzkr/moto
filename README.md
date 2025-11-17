# Moto: Modern Bootloader for Quansheng UV-K5 (V3) (and Variants)

Moto (Japanese "もと / 元", meaning "origin") is a modern bootloader for UV-K5 (V3) and variants (UV-K6, UV-K1, etc).
Moto mounts the radio as a USB disk on a computer, providing a drag-and-drop user experience for firmware or data flashing and backups.

Features: 

- Radio as USB disk to provide a drag-and-drop user experience
- Firmware readout and flashing
- Data storage (SPI flash) read and write
- FAT file system
- UF2 firmware file format
- :x: No serial commands support

## Usage

Moto will replace the original bootloader, so special tools are required to flash it to the radio. It is recommended to use [Ichi](https://github.com/muzkr/ichi), a sister project of this one, which is specifically designed for updating bootloaders.

Just like the stock bootloader, holding down the PTT button while powering on will enter Moto's DFU mode. 

Connect the device to your computer using a USB Type-C cable. A disk drive labeled "MOTO" will appear on your computer. This disk represents the internal storage space of the radio. Inside the MOTO disk, you will find:

- CURRENT.UF2: the current firmware
- DATA.UF2: the data storage (2MB)

These files can be copied to a safe place as backups.

Moto supports writing (flashing) firmware or data in the UF2 format. Copy the UF2 file that needs to be updated to the device to the MOTO disk, and Moto will write firmware or data to internal flash or SPI flash based on the address specified by the UF2 metadata. 

Due to system resource constraints, Moto allocates the 2MB data storage into 8 separate regions (256KB each), allowing writing to only one region at a time.

For detailed instructions, please refer to the wiki.


## License

Apache 2.0

Third-party components and copyright information can be found in the 3rd_party_licenses.txt file.
