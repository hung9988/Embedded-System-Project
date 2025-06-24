# HUST Embedded Systems Project
This is a project for the course of Embedded Systems at HUST.

## Project Overview
This project focus on the design of a analog macro pad using the Blackpill (STM32F411CEU6) microcontroller. 


## Features
- **Analog Output**: The macro pad can output analog signals.
- **Rapid Trigger**: A feature that allows the macro pad to send a signal immediately when a key is pressed, without waiting for the key to be released.
- **USB HID**: The macro pad will be recognized as a Human Interface Device (HID) by the host computer, allowing it to send key presses and other input events.
- **USB CDC**: The macro pad will support USB Communication Device Class (CDC) for serial communication with the host computer.
- **Macro Support**: The macro pad can be programmed to perform specific actions when certain keys are pressed.
## External Links
- [3D Case Design](https://cad.onshape.com/documents/5e6cfea2e32c850f530cae58/w/579554a927bd0a7730e0b539/e/56addd2516469d5755cfe071?renderMode=0&uiState=685a2b979b232715eddfcd3f)

## Resources

- [How does USB work?](https://www.youtube.com/watch?v=N0O5Uwc3C0o&t=158s)
- [What is Rapid Trigger?](https://www.youtube.com/watch?v=eZENx1T7OLw)

## References

- [Minipad](https://github.com/minipadKB/minipad-firmware)
- [Riskeyboard](https://github.com/riskable/void_switch_65_pct)
- [Macrolev](https://github.com/heiso/macrolev/tree/main)
- [TinyUSB](https://github.com/hathach/tinyusb)
- [USB in a Nutshell](https://www.beyondlogic.org/usbnutshell/usb1.shtml)
- [SSD1306](https://github.com/afiskon/stm32-ssd1306/tree/master)

