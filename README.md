
# FreeRTOS robot control with TCP Server and battery management.

Basic example using FreeRTOS as main RTOS for multiple task management in order to controll the robot and get an indicator LED of the battery status, using a 32bit microcontroller ESP32.

![](https://github.com/anycam/FreeRTOS-Robot/blob/master/Images/PCB.png)
![](https://github.com/anycam/FreeRTOS-Robot/blob/master/Images/Block.png)

## Install ESP-IDF environment
[Follow the steps clicking here.](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)

## How to compile and use example on linux with ESPRESSIF ESP-IDF environment

-Open a terminal in your project directory example -> "~/esp/tcpbat" \
-Once you are located on your terminal in the project directory, start the idf global path with the next command -> ". $HOME/esp/esp-idf/export.sh"\
-Set the target and open the menu config in order to configure your WIFI conection name and password -> "idf.py set-target esp32" and "idf.py menuconfig"\
-We are almost done! Now build your project (rub your hands together for luck) and enter -> "idf.py build"\
-If everything is ok, we are ready to upload our code into the target by writing -> "idf.py -p /dev/ttyUSB0 -b 460800 flash" What does this means? Ok its easy, first -p indicates the port, if you don't know your ESP32 port enter "ls /dev/tty*" to see the PORT where your ESP32 is connected by plugin and unplugin your board, -b indicates de baud rate but almost all the ESP32 boards supports 460800 which is just fine for us and quick, finally flash tells what it is, FLASH IT!

That's it, you should be able to see the percentage of the uploading process until the 100%

### See the information at the serial monitor
```
Open the serial monitor of your choice and select your port with 115200 as baud rate for communication.
You should be able to see you IP and PORT for the conection and battery or power status.
```

### Running the app with TCP client using TCP tools

```
Open TCP tools on your smartphone, select TCP client and enter the server IP and Port.
Then you can type "a,1" and hit enter to see how the motors moves forward, feel free to execute the commands 
of the main app.
```
### What to see?
```
By sending the commands to move, you can see your motors moving or your LED's blinking in the same order you typed.
Also the LED attached to the GPIO27 will be blinking slowly if the battery percentage is more than 50%, toherwise it will 
starts to blink faster (yeah, this battery monitor should be better) but it is a good way to test and create Tasks in the FreeRTOS.

Feel free to modify and share anything here.
```
