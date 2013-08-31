Some notes about the original hardware we're hacking up here...

Parts on the control board:

- SOIC-8 I2C EEPROM, marked:

        Atmel306
        24C02BN
        SU27 D

- Unmarked SOIC-8 IC
    (presumed MCU, not able to identify by pinout)

- n-channel enhancement mode mosfet (si2302)
    - marked `A2sHB`
    - smd "331" (330 ohm) resistor on gate of each, in series to mcu
    - no pull-down on gates... the thing flashes when plugged in

- regulator
    - marked `78L05 d3`
    - rated 5V, 100mA
    - 100 uF on input, 47 uF on output

- IR receiver
    - very clearly marked `3144S` but i can't find a datasheet or any other info about it
    - mated to a ~4" cable via a very small pcb inside some heat-shrink (pcb is purely structural, no logic or additional components)
    - works at 3.3v

- Other parts:
    - capacitor, 12 pF
    - capacitor, 1 nF
    - capacitor, 100 nF
    - resistor, 15 ohm (appears to be 2W carbon-film type, or maybe 5W metal-film)
    - inductor/ferrite bead (probably, it's in series from regulator output to 5v power)

Other observations (some stuff i could see with my multimeter, and some calculations based on the assumption that the driver's claimed 600mA current regulation is correct):

 - light off:
    - ~48 kHz ripple (at + side of LED module)
    - input voltage 12.12V

- light full-on white:
    - input voltage 8.75
    - 155.3 Hz pwm on red, green - blue appears to be fully on
    - voltage drop across:
        - red:   6.21
        - R+red: 8.63
        - R:     2.42
        - green: 8.63
        - blue:  8.65
    - calculations:
        - total power:
            5.25 W
        - current through:
            - R, red:   161 mA
        - power through R: 390 mW
    
- light full-red:
    - ~ 138 kHz ripple
    - 155.3 Hz pwm on red, ripple on others
    - input voltage 11.68
    - voltage drop across:
        - red:   6.72
        - R+red: 11.45
        - green: 0.00
        - blue:  0.00
    - calculations:
        - total power (assuming 100% duty, which I think is wrong):
            7 W
        - current through:
            - R, red:   315.33333 mA
        - power through R: 1.49 W

- light full-green:
    - ~ 100 kHz ripple
    - 155.3 Hz pwm on green, ripple on others
    - input voltage 10.15
    - voltage drop across:
        - red:   4.51
        - R+red: 4.50
        - green: 9.94
        - blue:  1.76
    - calculations:
        - total power (assuming 100% duty, which I think is wrong):
            6.09 W

- light full-blue:
    - ~ 100 kHz ripple
    - momentary 155.3 Hz pwm on blue, then 0Hz (ripple on others).
    - input voltage 9.82
    - voltage drop across:
        - red:   3.68
        - R+red: 3.60
        - green: 6.28
        - blue:  9.68
    - calculations:
        - total power: 5.892 W



