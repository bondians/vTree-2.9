vTree 2.9
==========

This is mokus's version of the XBee-controlled RGB LED floodlight.  This repository includes the latest hardware and firmware designs.  If cayuse is interested the firmware can be fairly easily ported to his hardware as well.

Hardware
=========

In the `schematics/` directory, there are several gEDA schematics and layouts.  The main ones of interest are:

 * `schematics/standalone-power-section`:  This is a small board with a power regulator, 3 MOSFETS, and associated resistors and capacitors.  Just add an MCU and IR receiver of your choice.

 * `schematics/xmega-xbee-rgb-led`: This is mokus's current prototype - it's a single board with all the stuff from the "standalone power section" plus an XMEGA 8E5 and headers to directly attach an XBee (non-pro only, for space reasons).

For cost reasons, all current hardware makes a pretty heinous compromise - the 3 LED channels share a single driver.  The plan is to attempt to correct for this in software (this is partially implemented and looks tractable).  If that proves infeasible, future versions may be modified to use a voltage-regulated supply and a separate 3-channel current regulation stage.

Firmware
=========

The firmware currently targets only the XMEGA E5, but should be pretty easy to port to any AVR.  It is also currently pretty simple, since I'm still mostly working on getting the hardware built.  It has 2 control inputs - an XBee module and an IR receiver.  The XBee (series 1 or 2, both work) is expected to be configured for 9600 baud, 8N1, API mode with escape codes.  The IR codes are hard-coded and the current values are probably not useful to anyone but me.

The XBee message format is very simple (and very likely to change soon).  Any message with 6 or more bytes is interpreted as a command to set the R, G and B channels (respectively) to the first 6 bytes of the message, interpreted as a series of big-endian 16-bit words.
