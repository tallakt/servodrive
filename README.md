# Servodrive

## Note 2016

You are probably better off not using this library and
rather do something based on the PRU in the beaglebone. Althesame
this project may contain some useful code examples for other uses.

Tallak

## Old readme

```
--------------------------------------------------------------------
Copyright (C) Chris Desjardins 2008 - code@chrisd.info
Copyright (C) Tallak Tveide 2010 - tallak@tveide.net



This driver is written for the Beagleboard (but the concepts should
apply easily to other platforms by changing the pin to gpio mapping).

The driver implements pulse width modulation (PWM) by pulsing the 
gpio signals which are connected to the signaling wire of a standard 
RC servo. Any pin on the expansion header may be used to generate the
PWM signal provided the MUX has been properly configured in u-boot
or linux kernel. If you do not want to change the MUX configuration,
as a starting point you can try the following pins which are confirmed
to work with standard angstrom demo image and u-boot:

  3 - 5 - 7 - 9 - 11 - 12 - ... - 23 - 24

The driver has been tested with Futaba and Modelcraft servos that are 
supplied power from the +5V and GND pins on the expansion header, 
and the signal provided directly from the 1.8V GPIO pin with no extra
voltage conversion. Other servos might such additional electronics.

The default pulse widths are:

  -100%    1100 ms
     0%    1500 ms
   100%    1900 ms

In addition the servos may be turned completely off (no pulse).


The driver uses udelay() which does active waiting, so expect around 
10% of your CPU time to be used as long as any servos are running. When
all servos are off, there is little overhead.

Controlling the driver: The driver is controlled by writing to the device 
file called /dev/servodrive0. Some examples are shown below:

  echo 3:100  > /dev/servodrive0
  echo 5:-100 > /dev/servodrive0
  echo 7:off  > /dev/servodrive0
  echo 9:50   > /dev/servodrive0

Running the above three commands will leave the servo connected to 
expansion header pin 3 at 100% clockwise, 5 will be at 100% counter-
clockwise while pin 7 wil reveive no controlling pulse. Pin 9 will be at 
50% rotation clockwise. If you later run the command:

  echo 3:off  > /dev/servodrive0

the servo at pin 3 will not move, but if a force pushes the servo 
away from the 100% position, it will not react by moving it back.
No power is used and there will be no jitter whatsoever.


--------------------------------------------------------------------

History: This code was originally found in Chris Desjardins project 
that included joystick control on a Windows host and a client server
architecture. In April 2010 I (Tallak Tveide) repackaged the code 
so that it would run on any expansion header pins and compile with 
bitbake, and the file interface was modified. The client-server parts
were also removed making this a servo driver only project that should
be reusable for other projects.
 
See: http://chrisd.info/beagleRC/

--------------------------------------------------------------------

Compiling: Copy the servodrive folder into the openembedded/recipes/
folder. Then run the following command:

  bitbake servodrive

After bitbake completes, you should find a file called 
servodrive_X.X_-rXX.X_beaglebaord.ipk. Copy this file across to your
Beagleboard and run the command

  opkg install servodrive_X.X_-rXX.X.ipk

To load the module, run the command

  modprobe servodrive

You should now see a file called /dev/servodrive0

For more information about Angstrom, bitbake and OpenEmbedded, check 
out http://elinux.org/BeagleBoard#OpenEmbedded

--------------------------------------------------------------------

Wiring: To drive two servos "out of the box" the following wiring is
required (for the example, pins 3 and 7 are used).

Servo A requires the following 3 pins, all of which are on the 
expansion header:
 * Pin 3 to servo signaling (usually white) - this is gpio 139.
 * Pin 2 to servo power (usually red) - this is 5volt DC.
 * Pin 27 to servo ground (usually black) - this is ground.

Servo B requires the following 3 pins, also on the expansion header:
 * Pin 7 to servo signaling (usually white) - this is gpio 137.
 * Pin 2 to servo power (usually red) - this is 5volt DC.
 * Pin 28 to servo power (usually black) - this is ground.

See http://www.fatlion.com/sailplanes/servos.html for a good 
illustration of servo wires. In general the rule is:

 * gpio to servo signaling wire
 * 5vdc to servo power wire
 * ground to servo ground


--------------------------------------------------------------------

A note about voltages: From what I have read servos are supposed to
have ~3volt control signals, beagle has 1.8volt gpios. For a while I
was considering some way to up the voltage, but after I tried servos
with 1.8volt control signals and I saw that they worked perfectly
fine, I instantly forgot all such considerations. :)

--------------------------------------------------------------------
```
