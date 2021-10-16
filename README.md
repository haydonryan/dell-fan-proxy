# Dell Fan controller


For R7515, R540 non hot swap fans.

# Background
I run an R7515 in my homelab. It's in a separate room upstairs (from my office) with the door open, I can hear it from my office downstairs.  It makes the room unuseable that it sits in, for anyhting else but storage.

At 43% FAN, I can barely hear it from downstairs, 35% is really not too bad, up close.

Looking into it, I've got some non dell hardware( like many others do in homelabs) that the idrac controller doesn't konw how to talk to.  So the idrac just ramps the fans up to compensate for that one device.

I also noticed that at idle (which the machine sits 90% of the time) there is a 3 degree C differential between inlet and outlet temp, with the cpu being nice and cool.  

# Why
Dell support told me I'm not using the server for it's usecase (Datacetner usage) - They're correct.
Don't want to modify the firmware (it's signed anyways)
Thought this would be an interesting project tom get more into electronics and control boards.

As these machines come out of datacenters people are going to want to lab them - let's give them the power to do that!

# Disclaimer
Will void your warranty
Could cause your server to overheat
Improper connection of fans can short out your motherboard!!!

No responsiblility taken for damaged hardware, loss of income, etc.



# Bill Of Materials:
- 6 Molex 2.00mm 2 Row, 6 Position Rectangular Receptical (Part # 0511100660) ~80c each https://www.digikey.com/en/products/detail/molex/0511100660/1144838?s=N4IgTCBcDaIOoFkCMAOArAdgwWgHIBEQBdAXyA
- 36 Molex Connector Socket 24-30AWG Crimp Gold (Part # 0503948054) 5c each https://www.digikey.com/en/products/detail/molex/0503948054/2404822?s=N4IgTCBcDaIAwFY4GYCcAWAHI9IC6AvkA 

- 6 Amphenol ICC Conenctor Through Header Vert 6 Ppos 2.00mm (Part # 98414-G06-06LF) https://www.digikey.com/en/products/detail/amphenol-icc-fci/98414-G06-06LF/1535247?s=N4IgTCBcDaIGwAYCcBaAzHMAOFA5AIiALoC%2BQA

- 1 Arduino Mega

- 24 AWG hookup wire - 5 colors preferred, but just be careful if you don't have different colors Cheaper on Amazon (https://www.amazon.com/CBAZYTM-Stranded-Flexible-Silicone-Electric/dp/B073RD76QD/ref=sr_1_22?dchild=1&keywords=24%2BAWG&qid=1606865665&s=hi&sr=1-22&th=1) 

- 6 TMP36 Temperature Sensors?? ~$1.50 https://www.digikey.com/en/products/detail/analog-devices-inc/TMP36GT9Z/820404

- Shrink wrap for wires
  - https://www.amazon.com/Eventronic-200pcs-Shrink-Tubing-Tubes/dp/B087JHB2CL/ref=sr_1_68?dchild=1&keywords=shrink+wrap+tube&qid=1606867076&sr=8-68
  - https://www.amazon.com/High-Heat-Shrink-Tubing-Tube/dp/B0722HN8SW/ref=sr_1_80?dchild=1&keywords=shrink+wrap+tube&qid=1606867076&sr=8-80
  - https://www.mouser.com/ProductDetail/Qualtek/Q2-F-RK4-1-16-11-6IN-39?qs=f6iYHOF0qgRLREW66g%2FPwg%3D%3D&gclid=CjwKCAiA8Jf-BRB-EiwAWDtEGuuEjQNy0toZvRLEwSUv5-1h-9RVMS_Smtxu-MWcHakHl9INQqu-WxoCvIEQAvD_BwE 


# Log

Want 24 AWG gauge wiring because the fans are rated for 3 amps. And only 24 gauge works with the connectors, and is within spec.   https://www.powerstream.com/Wire_Size.htm



By measuring and pulling pins out of the fan, I was able to determine that the pins (with the plastic nib up, facing away from you in the order:

```
1   2   3
4   5   6
```

That the Wires are:

1. Blue - PWM
2. Not connected
3. Black - Ground
4. -| Red - 12v (this wire continues to fan)
5. -| Red - 12v Loopback
6. Yellow - Sense wire.

I found this forum post from someone who also got delta fans out that were used in a Dell server. 
https://www.eevblog.com/forum/projects/how-do-i-drive-these-delta-fans-with-varying-speed/

## April 12 2021
Connected the server fan up to the test bench and using a 10k pullup resistor got a nice square waveform that corresponds to the frequency.  

555hz at full speed
12.2v (same voltage as vcc)
The pullup resister can be placed on VCC of the arduino insted, and that tthen will pull it up to 5v in stead.
https://linustechtips.com/topic/1016644-arduino-hall-sensor-pull-up-resistor/

https://www.youtube.com/watch?v=YBNEXWp-gf0 Found this great video on high frequency Arduino PWM signals


Connected up and ran the straight output code  and got a max of 16530RPM
Once I connected the PWM signal and scoped the tach output, I got a ton of noise appearing on the tach line.  This is introducing errors in the count functions.

## April 16 2021
Low pass filter 
10k ohm resister
15 farad - not enough filtering
331= 330pf sweet spot
104 (100000pf) seriously rounds the square wave. 

2200pf lots of noise

Using just a 220 k ohm resister from the signal made the difference ( no capacitor)
https://forum.arduino.cc/t/controlling-12v-pwm-fan-reading-speed-tach-readout-issues/863076

## August 4 2021
Did some more research last night on continuing the project.  Found this forum thread: https://forum.arduino.cc/t/problem-measuring-4-pin-fan-rpm/666804/10.

Also realized that I need to refactor the RPM sensing code as it loops and waits for the Tach count to go up - that's not helpful for also trying to output a tach signal back to the computer.  The tach output (when pulled up) is a nice square wave


## August 10 2021
removing the existing wiring from the arduino so I can test PWM input from the computer.
arduino -> breadboard
ground (orange) -> ground line
5v (brown) -> resistor

arduino -> fan
pin 9 blue

arduino -> resistor
pin 2 - purple 

There are different ways of reading the PWM signal from the computer. Because PWM is 25khz and in theory shouldn't change often - doing a blocking pulseIn seems to be ok.  This way we avoid using interrupts that we're using for checking the fan RPM.  

18000rpm = 300rps. Two ticks per rotation, gives 600hz
Max fan speeds at 100%  (tested in idrac) 16920, 16920, 16800, 17040,16920 rpm

## August 13 2021
Today I got to the milestone of intercepting the fan signal from my desktop and applying the fan map to it, then using that to run the server fan.  Reading of the fan rpm, along with faking the tach signal back to idrac all works.  

Next up is to make some harnesses and test with one fan in the server.

## September 7 2021
Decided to go the route of migrating the code to the atmega board.  Created a makefile and helper script to use the command line insted of the GUI.  Google tests for some of the code would be an interesting exercise too.

Next up I'm going to expand this to 6 channels on the board before testing in the server.  I've also started thinking what the actual hardware solution would look like if I made a PCB.


## September 11 2021
Expanded the data structures up to 6 fans.  

Running into issues around the fan PWM driver - I don't follow how the timers work with PWM signals - so need to do some research on that before continuing.  I guess I could try reading in teh signals that don't require PWM from other pins, but it feels like doing the hardest part first would be smarter.
Arduino Pin	 Register
2	OCR3B
3	OCR3C
4	OCR4C
5	OCR3A
6	OCR4A
7	OCR4B
8	OCR4C
9	OCR2B
10	OCR2A
11	OCR1A
12	OCR1B
13	OCR0A
44	OCR5C
45	OCR5B
46	OCR5A

source: http://astro.neutral.org/arduino/arduino-pwm-pins-frequency.shtml

## NEXT

https://arduinoinfo.mywikis.net/wiki/Arduino-PWM-Frequency

## Sat Sep 25 2021 
Expanded the program to output on 6 separate PWM channels. Meaning we're getting close to doing some hardware testing.
Calculated each fan at 100% uses 20W of power. - or 120W for the whole machine.

Pin 20 and 21 for reading the fan tach - works by just wiring the signal pulled high directly in.

I fixed a couple of bugs in the mapping code - it didn't work for a zero value.  Not good, but oh well. Fixed at least.  Should be ready to try the hardware for tomorrow in the server.

## Wed Sep 29 2021 
Tested the fan project in the server last night - and it worked really well. Next up - test with 6 channels populated - meaning that I need to make 5 more adaptors.  I also realized that the external USB doesn't provide power while the motherboard is off, so I might not be able to use that to provide power to the board since one of the central fans spins while the server is off.

## Sat Oct 16 2021 
Was able to plug the fan project in and use VIN to power the arduino.  I did notice that power was going back into the computer though via the VIN when the usb was plugged into my laptop. Looks like I need a 1N4000 family diode to stop current going back into the computer from the arduino.  I have a bunch of 1N4007 in one of the starter kits.
Also realized that the minimum speed of 0 doesn't work well when the computer is off but IDRAC is on, so I changed the minimum speed to 20% this seems to work nicely with it off, and on - provides a reasonable amount of air, while not being super loud.

I can't seem to get SDA /SCL reading the fan tach signal correctly - tried pulling them high with another 10kohm resistor, as well as different configurations (didn't try a pull down resistor though).  I ended up posting the question to reddit as it's beyond my level of understanding


# Making cable tips.
Put some shrink tubing on the 12v wire on the loop and the main wire before putting the terminal into the block. Stray 12v is SUPER bad.
Cut the termianl off the tape but leave the small bit of tape on for hodling it steady.
put the termianl into the 1.3mm crimp size first all the way up to the tape - it should hold iself in there - making it easier to feed in the cable(s).

Do the dual power lead crimp first (it's the hardest)

# Possible improvements
It would be much safer to map the fan speed returned to idrac from the detected speed of the fan, not the requested rpm. That way if a fan fails, then idrac will throw a fault.

# Resources:
Starting point: https://www.youtube.com/watch?v=UJK2JF8wOu8
Replacement Fan: https://www.itcreations.com/product/119156
Standard 4 pin fan pinout https://allpinouts.org/pinouts/connectors/motherboards/motherboard-cpu-4-pin-fan/
https://pastebin.com/s49SWw1A
https://github.com/Max-Sum/HP-fan-proxy/blob/master/hp_fan.ino
https://www.avdweb.nl/arduino/misc/handy-functions
https://www.reddit.com/r/homelab/comments/8wbogx/my_attempt_at_creating_a_circuit_board_that/
https://www.reddit.com/r/homelab/comments/beuks5/silencing_the_hp_dl380_for_good/

Setting the interrupt priority: https://www.youtube.com/watch?v=H3o08WE0zEI

https://homeservershow.com/forums/topic/7294-faking-the-fan-signal/

https://forum.arduino.cc/t/problem-measuring-4-pin-fan-rpm/666804/6
http://www.benripley.com/diy/arduino/three-ways-to-read-a-pwm-signal-with-arduino/
https://www.youtube.com/watch?v=nut1ZiiXzVU&t=143s
