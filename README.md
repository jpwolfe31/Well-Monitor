# Well-Monitor
Arduino Well Monitoring System using MKR1010 and Blynk for Water Wells

This well monitor was my first Arduino project.  We had recently moved into a house that had an elaborate well and irrigation system.  It was difficult to determine how to optimize the system to maintain a consistent holding tank water supply as there was no way to collect operational data.

This monitor collects data from three well pumps, one pressure pump, a city water helper valve and a pressure tank pressure sensor.  The monitor records this data every 5 seconds to a SD card large enough to hold data for many years.  Reports can be run against the data on the device, and the data can be exported to an excel spreadsheet using a computer and SD card reader.  

Originally, the data was only accessed through a computer or a menu system on the device that included two buttons (scroll and select) and the OLED display.

Since then, I have connected the app to a Blynk IOT iPhone app and have not had a need to use this menu system any further.   I send messages through the Blynk app to the device using the Blynk terminal widget which then provides data and reports back.   Each day, the Blynk app also reports by email minutes on for each pump and valve and the lowest pressure recorded.  The app also sends an alert and email if the well system has low pressure or if it has started to use city water.  

With this well monitor, I have been able to schedule the irrigation system to maintain well water in the holding tank to avoid any use of city water.  

If I were building this project again, I would use a Nano-ESP32 and a separate SD module and leave off the OLED display and pushbuttons.
Pardon the code.  It is a bit messy given that it started with a menu system, the OLED display and push buttons and now has the Blynk system overlayed as well.
Unfortunately, Blynk is not supporting new makers on its app right now, but I believe this program could be modified to work with the Aurduino Cloud IOT and its messenger widget, although the reports would need some reformatting.

See folders above for code, schematics and build information.  Feel free to email me at jpwolfe31@yahoo.com if you have any questions.
