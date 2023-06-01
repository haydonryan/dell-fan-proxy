# Dell Fan controller
<img src="https://raw.githubusercontent.com/haydonryan/dell-fan-proxy/main/Images/IMG_1392.jpg" width="200" height="400">   <img src="https://raw.githubusercontent.com/haydonryan/dell-fan-proxy/main/Images/IMG_1393.jpg" width="200" height="400">   <img src="https://raw.githubusercontent.com/haydonryan/dell-fan-proxy/main/Images/IMG_1590.jpg" width="200" height="400">   <img src="https://raw.githubusercontent.com/haydonryan/dell-fan-proxy/main/Images/IMG_1591.jpg" width="200" height="400">

Arduino Mega Fan Controller Sheild For R7515, R540 non hot swap fans.

# Background
I run an R7515 in my homelab. It's in a separate room upstairs (from my office) with the door open, I can hear it from my office downstairs.  It makes the room unusable that it sits in.

At 43% FAN, I can barely hear it from downstairs, 35% is really not too bad, up close.

The minimum fan speed and therefore noise is designed for a datacenter next to other heat generating servers with no noise contstraints, not a room in a house.
Plus I've got some non dell hardware( like many others do in homelabs) that the idrac controller doesn't know how to talk to, so the idrac just ramps the fans up to compensate for that one device.

I also noticed that at idle (which the machine sits 90% of the time) there is a 3 degree C differential between inlet and outlet temp, with the cpu being nice and cool.

# Why
- Dell support told me I'm not using the server for it's use case (Datacenter usage) - They're correct.
- Don't want to modify the firmware (it's signed anyways)
- Thought this would be an interesting project tom get more into electronics and control boards.

- As these machines come out of primary service in datacenters people are going to want to lab them - let's give them the power to do that!

# Disclaimer
- Will void your warranty
- Could cause your server to overheat
- Improper connection of fans can short out your motherboard!!!

- No responsibility taken for damaged hardware, loss of income, etc.



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


# Random bits of information

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

Arduino Pin	 Register
- 2	OCR3B
- 3	OCR3C
- 4	OCR4C
- 5	OCR3A
- 6	OCR4A
- 7	OCR4B
- 8	OCR4C
- 9	OCR2B
- 10	OCR2A
- 11	OCR1A
- 12	OCR1B
- 13	OCR0A
- 44	OCR5C
- 45	OCR5B
- 46	OCR5A

source: http://astro.neutral.org/arduino/arduino-pwm-pins-frequency.shtml


## Fri Dec 31 11:26:19 2021
Checked the continuity - on the new server board - there's a common 12v rail AND common 12v ground.  This means that I can easily check the fan project using the bench power supply.

So far so good on the server. Printed U shaped spacers to save the cables for fans 1-3.  Definitely prefer to do this over not having anything pushing the top up.

I did find that only fan 3 has 12v power when powered off, the rest must be switched somewhere in the motherboard.

Cable lengths:
- Fan 1 30cm
- Fan 2 26cm
- Fan 3 45cm
- Fan 4 40cm
- Fan 5 35cm
- Fan 6 40cm

Max length shrink tubing at the arduino end: 1-1.5cm

# July 2022 Update
I've been testing this during summer and noticed that as the inlet temperature approaches the limit "for this configuration" the fans ramp significantly.  This is clearly a preventative reaction rather than a reactive one.  Also ensuring that the hardware that is in the server is "compatilbe" aka listed on the compatabilty matrix certainly helps to reduce the speed of the fans.


# Making cable tips.
Put some shrink tubing on the 12v wire on the loop and the main wire before putting the terminal into the block. Stray 12v is SUPER bad.
Cut the terminal off the tape but leave the small bit of tape on for holding it steady.
put the terminal into the 1.3mm crimp size first all the way up to the tape - it should hold itself in there - making it easier to feed in the cable(s).

Make sure to crimp the actual insulation.
Don't have the wire come out too far as mounting this, the pins will potentially pus the pin out if there's not enough space.
https://www.youtube.com/watch?v=Ta55NTSBLN0


Do the dual power lead crimp first (it's the hardest)

# Possible improvements
It would be much safer to map the fan speed returned to idrac from the detected speed of the fan, not the requested rpm. That way if a fan fails, then idrac will throw a fault.

# Resources:
- Starting point: https://www.youtube.com/watch?v=UJK2JF8wOu8
- Replacement Fan: https://www.itcreations.com/product/119156
- Standard 4 pin fan pinout https://allpinouts.org/pinouts/connectors/motherboards/motherboard-cpu-4-pin-fan/
- https://pastebin.com/s49SWw1A
- https://github.com/Max-Sum/HP-fan-proxy/blob/master/hp_fan.ino
- https://www.avdweb.nl/arduino/misc/handy-functions
- https://www.reddit.com/r/homelab/comments/8wbogx/my_attempt_at_creating_a_circuit_board_that/
- https://www.reddit.com/r/homelab/comments/beuks5/silencing_the_hp_dl380_for_good/

- Setting the interrupt priority: https://www.youtube.com/watch?v=H3o08WE0zEI
- Delta Fan Datasheet for similar fan: https://www.mouser.com/datasheet/2/632/dele_s_a0011169984_1-2290125.pdf
- https://homeservershow.com/forums/topic/7294-faking-the-fan-signal/

- https://forum.arduino.cc/t/problem-measuring-4-pin-fan-rpm/666804/6
- http://www.benripley.com/diy/arduino/three-ways-to-read-a-pwm-signal-with-arduino/
- https://www.youtube.com/watch?v=nut1ZiiXzVU&t=143s
