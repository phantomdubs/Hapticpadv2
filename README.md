# CNCDan - Haptic Pad
![Alt text](title.png "Haptic Pad")

A 6 button macropad with a display for button labels and a mouse knob with haptic feedback!

[Project Video Link](https://youtu.be/bNUKRJQjuvQ)

![Haptic Pad v2.0](Pictures/IMG_1243.jpg "Haptic Pad v2.0 Powered On")

#### 🆕 New Advanced Features (v2.0)

*   **USB Drive Mode**: Edit your SD card files directly from your PC! Hold **Button 5 (Center Bottom)** while plugging in the USB cable to mount the internal SD card as a flash drive. 
*   **Reboot Shortcut**: While in USB Drive Mode, simply press **Button 1 (Top Left)** to exit and reboot the pad back into normal mode.
*   **Smart Sleep Mode**: To preserve the OLED display and save power, the pad automatically enters sleep mode after 5 minutes of inactivity. Press any button to wake it up.
*   **Per-Profile LEDs**: LED colors and modes are now tied to your active profile. Your pad can now automatically change its "look" whenever you switch between apps or games.
*   **Enhanced UI Layout**: The screen layout has been redesigned for better legibility. The profile name has been moved from the center to the edge of the screen, freeing up space for **larger 24x24 pixel icons** (upgraded from 16x16). 
*   **Diagnostic Tools**: Includes a `HardwareTester.uf2` file to quickly verify your wiring and motor directions before loading the main firmware.
*   **Custom Icon Support**: New Python scripts provided in the repository allow you to convert your own images into the exact bitmap format required for the OLED display. (**Note**: These scripts are intended as examples for icon creation and are not full-featured tools).

#### 🛠️ Design Improvements (v2.0)

The 3D files in this repository have been updated with several quality-of-life improvements:
*   **Choc Keycap Support**: The housing now features updated square keycap holes designed to fit standard **Kailh Choc low-profile keycaps** (available on Amazon/AliExpress) instead of being limited to custom 3D printed ones.
*   **Pico Button Access**: The bottom plate now includes two access holes so you can press the physical **Boot** and **Reset** buttons on the RP2040-Plus board without disassembling the case.
*   **Better Magnet Fit**: Magnet pockets on the knob and the knob interface plate have been slightly enlarged for a much better "press-fit" experience.

> [!IMPORTANT]
> **Assembly Note**: If you are using standard Choc keycaps, the **bottom-center keycap** must be trimmed down on its underside. This is necessary to prevent it from interfering with or damaging the LCD screen's ribbon cable during use.

#### 💡 Technical Notes & Troubleshooting

*   **Display Stability**: It is highly recommended to install the **10nF capacitor** (C1) on the controller board. This was optional in earlier versions but has been found necessary to reduce noise and prevent display flickering.
    ![Capacitor C1 Location](Pictures/IMG_1246.jpg "Capacitor C1 on RP2040-Plus")
*   **Voltage Logic Fix**: While the OLED display is rated for 5V, it can behave erratically when receiving 3.3V logic signals from the Pico. To fix this, **add a diode inline** with the 5V power supply to the display. This drops the voltage to ~4V, allowing the 3.3V logic signals to communicate reliably with the display.
    ![Inline Diode Fix](Pictures/IMG_1245.jpg "Inline Diode for Voltage Drop")
*   **Hardware Tester Bug**: When running the knob test in `HardwareTester.uf2`, the mode names displayed on the screen (Clicky, Twist, Momentum) may not match the actual haptic mode being tested. As long as you feel three distinct haptic behaviors as you cycle through, your motor and encoder are working correctly.

#### Bill of Materials

For a complete list of components and purchasing links, please refer to the **[Parts List.csv](Parts%20List.csv)** file included in the root of this repository.

#### Printing Instructions

#### - Printed version
Print all files in the **3D Files/STL's** folder. You will need:
*   6 of the keycap file (**Note**: Only required if you are NOT using the updated v2.0 housing).
*   Two of the Menu button file.
*   Two of the PCB spacer file.

#### - Machined Version
Print all files in the **3D Files/STL's** folder except: "Custom Keycap.STL", "Macro Pad - Printed Version.STL" and "Menu Button Printed.STL".
Get all of the .STEP files in the **3D Files/STEP** folder machined. Don't include the Macropad Assembly .STEP file from the main directory as it is a complete model of Macro Pad rather than an individual part.

---

### Software

> [!TIP]
> **Recommended First Step**: Flash the `HardwareTester.uf2` file found in the `HardwareTester` folder first. This allows you to verify all your buttons, the display, and the motor encoder are working correctly before loading the full MacroPad firmware.

#### Flashing the Firmware
If you don't wish to compile the code yourself, just copy the latest version of the `MacroPad_V2.0.uf2` file from the software folder and install it directly onto the memory of your Pico using the boot method shown in the video.

You will also need to have your SD card set up correctly in order to use the macro pad.
Copy the entire contents of the **Example SD Card** folder onto you SD card to begin with. This folder now includes **6 extensive example profiles** (`StarNavC`, `Mech-T`, `CybMix-M`, `Alch-C`, `Hack-T`, and `Race-M`) to help you get started.

### XML Config

In the `<Settings>` tag of the XML file you will find the P and I tuning values for the various wheel modes.

#### Per-Profile Settings
Unlike previous versions, the LED settings are now stored **inside each profile**. This allows you to have unique lighting for every application.

*   `<LED_Mode>`: Acceptable inputs are `Breath`, `Bands`, `Halo`, `Rainbow`, `Solid`, and `Off`.
*   `<LED_Primary>` & `<LED_Secondary>`: Colors formatted as R,G,B (0-255). 
*   `<LED_Brightness>`: Set the intensity (0-255).
*   `<WheelMode>`: `Clicky`, `Twist`, or `Momentum`.
*   `<WheelKey>`: Any key value from [keycode-visualizer](https://keycode-visualizer.netlify.app/) to be held while the wheel is moving.

#### Macro Buttons
Each profile contains 6 `<MacroButton>` tags:
```xml
<MacroButton>
    <Action>0,68</Action> <!-- Delay(ms), KeyCode -->
    <Action>0,0</Action>
    <Action>0,0</Action>
    <Label>Dimension</Label>
</MacroButton>
```
Each action has two values: the delay (in ms) followed by the keycode. Setting both to 0 skips the action.

---

### Hardware Assembly & PCB's

![Exploded Motor Assembly](MotorStack.png "Motor Stack Assembly Diagram")

*(See the original repository for detailed wiring diagrams and PCB assembly guides)*

![Assembled Internal Wiring](Pictures/IMG_1244.jpg "Internal Wiring View")
![Assembled Top View](Pictures/IMG_1105.jpg "Haptic Pad Assembled (Top)")

You will need to have both PCB's made to complete this project. Zip files for manufacturing can be found in `PCB's/MacroPad` and `PCB's/MacroPad Controller Board`.

**Controller Board**: The SD reader should be flat against the PCB. The motor controller gets installed on pin headers with the "bottom" face up. The Pico should be soldered directly with its USB port facing outwards.

**Main Board**: If including LEDs, install them first! Match the arrow rebate to the "1" pad. All "C" components are 0.1uF 0603 capacitors.

**Connecting**: Boards are connected via direct wiring. Match the labels on both boards. The three motor wires connect to U, V, and W (order can be swapped if direction is wrong). Encoder connections are also labelled—only the four main pins are required.

> [!TIP]
> **Screw Installation Hack**: The original design calls for tapping the plastic for M2 screws. If you don't have an M2 tap, you can simply use a hot soldering iron to carefully press the screws into the pre-sized holes in the plastic. This melts the plastic slightly around the threads, creating a very secure hold once cooled.
