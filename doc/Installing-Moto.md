
Moto is a bootloader. Given this premise, Moto must reside within the bootloader section of the MCU's internal flash memory. This section is originally occupied by the stock bootloader, such as when the radio leaves the QS factory.

```
               MCU internal flash
    0x08000000 +----------------+
               | Bootloader     | 10 KB
      08002800 +----------------+
               |                |
               | Firmware       | 118 KB
               :                :
               |                |
      08020000 +----------------+
```

Most of the time, the bootloader remains unnoticed. When you turn the power/volume knob on, the bootloader is the first to run, and its primary duty is to boot the firmware. End of story.

Occasionally, specifically when updating the firmware, you press and hold the PTT button while powering on. At this particular moment, the bootloader becomes slightly noticeable—one of the signs being that the flashlight comes on. Next, you use a tool (either an .exe or web-based) to send the firmware to the bootloader, which then writes the firmware into the firmware area of the internal flash memory.

But how do you install/update the bootloader itself? Those with specialized technical skills may use a hardware debugger and the openocd frontend to directly write the bootloader to the correct location within the internal flash. However, for regular users, a simpler method is needed.

The following is intended for regular users.

The same author of Moto created [Ichi](https://github.com/muzkr/ichi), a tool specifically designed for updating the bootloader.

Ichi itself is a firmware. This means you install it in the same way you would install any other firmware. Regardless of your current bootloader (stock bootloader or a version of Moto), install Ichi using that method.

After running Ichi, connect the radio to your computer via USB. A new disk labeled ICHI will appear on your computer. Copy the Moto release in UF2 format to the ICHI disk. That's all—Moto is now installed on your radio as the bootloader. You will use Moto to install firmware thereafter.

When a new version of Moto is released, use the same process to update it.

Let me put it this way: once you understand the underlying workings, the installation or upgrade operation itself is so simple it's hardly worth mentioning.

## Restore Stock Bootloader

To restore the stock bootloader, the Ichi repository's [archive](https://github.com/muzkr/ichi/tree/main/archive) folder contains several stock bootloaders in UF2 format. You can restore it using the same procedure as above.
