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
10 ohm resister
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


