# Esp32 Round Display for a digital gauge cluster

This project is source-available and free for personal and educational use.
Commercial use is NOT permitted.

### *****Please do not use these files to sell this to others.  I made these so that those wanting to modify their dash/cluster dont have to spend an arm and a leg to do so.***** ###

Youtube Tutorial/playlist: https://youtu.be/t7H6pevep40

1. Install [visual studio code](https://code.visualstudio.com/Download)
2. Install the ESP-IDF extension(shown here: https://www.waveshare.com/wiki/ESP32-P4-WIFI6-Touch-LCD-3.4C#Introduction_to_ESP-IDF_and_Environment_Setup_.28VSCode_Column.29)
    * Open VSCode Package Manager
    * Search for the official ESP-IDF ide extension
3. Open this repo folder in VS code
4. Plug in Esp32-P4/screen via usb c
5. Set target device to esp32p4(see "Description of Bottom Toolbar of VSCode User Interface" in the waveshare link above)
6. Build 

7. Flash
<br>


# CANBUS
For canbus integration, I have added a few CAN protocols(haltech, hondata, etc).  If you would like more added please join the discord and provide the can protocol and I can add it quickly
<br>
<br>
You will need to purchase a [small can transciever](https://a.co/d/09CiRq2o) for 9$.  With it, you can ignore any other sensor wiring.  If you need help with wiring those two wires, again join the discord.

<br>
Also, the sensor source on line 37 of the main.c file needs to be updated to 
```SENSOR_SOURCE_CAN``` as the default for this code is the analog/non-canbus sensors

## Issues/bug fixes

### 4in waveshare round screens
For 4in round screens, the base tach image needs to be updated as the resolution is 720x720 and not 800x800 like the 3.4in screens.
<br>
To do that, just replace the ui_img_1656279599.c file in main/tach_ui/images with the file found here: https://drive.google.com/file/d/1_PrP6jOna2s5Ua82ol2qqV2OZWf9c4_l/view?usp=sharing
<br>

### Older boards/revision errors

If you see an error like:  ```A fatal error occurred: bootloader/bootloader.bin requires chip revision in range [v3.1 - v3.99] (this chip is revision v1.3). ```, you need to update the minimum support board with a just a few clicks.
<br>
Follow these instructions here: https://discord.com/channels/1474501462905192450/1474502475280154927/1480043568469901459
