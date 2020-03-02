issue with lockup in samd21 sleep mode:

https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode

see in that thread: 

> The solution for the customer is to disable SysTick interrupt before going to sleep and enable it back after the sleep.

also this re: RTCZero library and SysTick:

https://forum.arduino.cc/index.php?topic=617395.15

RTCZero hasn't merged the patch, but ArduinoLowPower has -- ?

https://github.com/arduino-libraries/RTCZero/pull/46

they say that arduinolowpower has resolved this issue

i believe (not sure) that the current issue with rtczero simply resets the hardware, so it's not too problematic at this point -- it just means that there will be a join 


