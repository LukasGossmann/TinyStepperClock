<img alt="Front view of the clock" src="Pictures/Front.png" width="50%">

**Description:**  
Tiny stepper motor based clock (including base, L=80mm, W=80mm, H=127mm)  
The clock face is 50mm in diameter with a 5mm wide clear / slightly opaque ring around it for a total diameter of 60mm.  
Behind the clock face is an LED ring with 12 WS2812 LEDs that light up every hour and play a short animation.  
The stepper motors used in this clock are very weak so a lot of care need to be taken to align the shafts properly and for the gears to be running as true as possible on the shafts.  
Each stepper motor has 20 fullsteps per rev which with a gear reduction of 3:1 gives 60 steps per rev.  
The minute hand takes a step every minute while the hour hand takes a step every 12 minutes.  
Since this is the tiny version this implies the existence of a bigger stepper motor based clock ;) (coming soon)  
**Waring:** The clock is mains powered and there are a few somewhat exposed live traces / metal contacts so build and use at your own risk.

**Building:**  
Create all parts needed as shown in the FreeCAD drawing.  
Assemble as shown on the pictures and in the FreeCAD drawing.  
A lot of care needs to be taken to be as precise as possible as there inst much room for tolerance in this design.  
Note that one of the stepper motors is wired in the opposite direction so that the same step sequence can be used to drive them.  
If the motors are turning in the wrong direction or both motors were wired the same on accident the code allows for inverting the motor direction as well.

**Tools:**
- **CNC machine** or other way to create parts from a piece of flat material as well as for engraving the clock face.
- **FDM or SLA printer** to create the spacer and the clear ring.
- Depending on the parts you can get and the need to modify them a **lathe** is very useful.
- Soldering iron, screwdrivers, etc.


**Stepper motors:**  
20 steps per rev  
3.3V @ 80mA per coil (gets slightly warm)  
Does not like having both coils energized at the same time, gets pretty hot and doesn't step well  
Diameter: 8mm  
Length: 9.2mm  
Gear (from factory): 10 teeth 0.2 module? (2.3mm diameter)  
Can be found from multiple sellers in china for very cheap (AliExpress)

**Stepper driver:**  
DRV8833 without current limiting (3.3V straight through)  
On a breakout board just soldered onto carrier PCB

**Clock hands:**  
Need to be made of a material that can be soldered to so they can be attached to the shafts.  
CNC milling them from some leftover PCB material works great.

**Power supply:**  
The Pi Pico is powered from a 230V to 5V mains power supply "brick" made by Hi-Link which can supply 600mA.  
To power the stepper drivers the 3.3V output of the Pi Pico is used. 
The stepper motors together draw around 160mA at 3.3V with only one of their coils being active at a time.

**Needed parts (mechanical):**
- 2x 60 tooth gear, 1.4mm thick, 0.3 module
    - one with 4mm hole and one with 2mm hole for the shaft
- 2x 20 tooth gear, 1.4mm thick, 0.3 module
    - with 2.3mm hole to fit onto factory gear of the stepper motor
- 2x Bearing 8mm OD, 4mm ID, 3mm thick
- 1x Bearing 7.3mm OD, 2mm ID, 3mm thick
    - 7mm OD bearings are also available, pick whatever is cheaper and modify the bearing holder design to fit your needs
- 3mm thick material for the frame parts (I used Resopal)
- Brass tube 4mm OD, 2mm ID (shaft for the hour hand)
- Brass tube 5mm OD, 3mm ID (used as spacers for the stacked PCBs)
- Brass rod 2mm (shaft for the minute hand)
- Various M2 and M3 screws, nuts and washers

**Needed parts (electrical):**
- 2x Stepper motor
    - order more than that because they tend to arrive broken (coil wires ripped off)
- 1x Raspberry Pi Pico without headers
- 1x 5V PSU (Hi-Link HLK-PM01 or pin and size compatible)
- 1x WS2812 LED ring with 12 LEDS (~50mm OD)
    - 8 LED version (~32mm OD) should work too with a modified spacer and a few code changes
- 1x Diode (shotkey or any other type, 1N4007 works just fine)
- 1x Capacitor (At least 5V rating, around 470µF works fine)
- 1x 2.54mm 2 pin screw terminal
- Some 2.54mm pin headers
- Thin wires
- The PCBs from the KiCAD project
