issue with lockup in samd21 sleep mode:

https://www.avrfreaks.net/forum/samd21-samd21e16b-sporadically-locks-and-does-not-wake-standby-sleep-mode

see in that thread: 

> The solution for the customer is to disable SysTick interrupt before going to sleep and enable it back after the sleep.

also this re: RTCZero library and SysTick:

https://forum.arduino.cc/index.php?topic=617395.15

RTCZero hasn't merged the patch, but ArduinoLowPower has -- ?

https://github.com/arduino-libraries/RTCZero/pull/46

they say that arduinolowpower has resolved this issue

i believe (not sure) that the current issue with rtczero simply resets the hardware, so it's not too problematic at this point -- it just means that there will be a join in the lora network when that happens

i believe that i did see this issue happen when looking at the hardware -- i thought i'd reset something inadvertenty but maybe it was the sleep hardware


--

looks like we can change the code to 'pull down' the leds when in sleep, so that might work for pin13 ... but likely not for the charging LED ... should look at the charging circuit ... is it powered by USB? can we use BAT instead? maybe not a problem to use BAT b/c the charging chip won't be powered when the USB is off?  need to double check this -- can re-mill a board if useful ... can also test differential power consumption with present board by just jumping pins i think 


this is the library that they were referencing, which at first glance looks good:

https://github.com/arduino-libraries/ArduinoLowPower

specifically, this: https://github.com/arduino-libraries/ArduinoLowPower/blob/master/examples/TimedWakeup/TimedWakeup.ino

not sure if there are unknown issues with arduinolowpower -- might make more sense to use the pull request fix 

so might just add that fix


