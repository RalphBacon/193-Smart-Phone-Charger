# 193 Smart Phone Charger
Android and Arduino Code for the Smart Phone Charger (user-selected limits on (dis-)charging)

Keeping your phone connected to your charger for long periods (during the day at work or overnight) will cause battery stress and shorten its life. This device will allow you set predetermined limits on when to stop charging and when to start again (percentage of battery power).

The code is split into two parts: 

1. The .apk (Android app) that runs on your phone. As this .apk is not in the Play store you must give explicit permission on your phone to install it. Maybe one day I will get this into the Play Store (which costs money).

2. The Arduino code that receives all the stats from the phone regarding battery charging level and which then decides to charge (or not) your phone. The hardware is simple to put together but you will need one of those new-fangled soldering irons and the ability to solder (mostly) through-hole components with a couple of exceptions.

Each code app is placed into underlying folders here with their own README file which describes the current state of play.

The schematic of the hardware (controlled by the Arduino Pro Mini \[think: Arduino UNO without the USB bit] , but that could really be any microprocessor) is in a separate folder along with the EasyEDA PCB design files.

This is a work in progress. The protoype is complete and can be seen working in this YouTube video:  
https://youtu.be/fvb3i4yV20s 
I shall be concentrating in getting the phone app completed (done-done) and the Arduino code completed too (done-done). See the video on what I am talking about when I say "done-done".

### Parts List
+ Pro Mini Module (like a UNO or Nano but without the USB socket). You can use one of those if you want but it won't fit on the PCB I've used.
+ HC-05 Bluetooth Module to talk to the Android phone
+ INA219 Current Sensor to measure charging current
+ Si4599 Dual MosFet SMD 8-pin chip that controls the power to the phone (easy to solder)
+ SSD1306 0.91" OLED screen for messages (I guess this is optional but so very useful)
+ A few resistors, capacitors, solder, pin headers - refer to the schematic

