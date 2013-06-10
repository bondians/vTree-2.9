Current protocol (will likely change...):

1 byte 'sync' (or hardware-level break), 1 byte size (excluding size, sync and crc), 1-256 bytes payload (size 0 is interpreted as 256), 2 bytes CRC.  CRC polynomial TBD, prolly whatever's in the AVR libs.

overview of payload types:

    * enumerate: need to work out details, but there must be an enumeration scheme whereby the host, when initializing, can discover all lights and any new light, when initializing, will report itself to the host.  Net effect being that the host will always know what devices are attached.  'Device number', used in many other message types, is established here (it may be negotiated or, more likely, it may be an NVRAM-stored attribute of the device).  Devices also should be able to determine whether there is a host present.  Simplest scheme would probably be for all devices to send a heartbeat, with "host" devices using a different range of device numbers (or just one specified device number if it's fair to assume there is always at most one host).

    * get device info: given device number, device replies with immutable info about itself such as number of channels (maybe also topology info about physical groupings), basic description of what each channel does (color/hue/sat/brightness/generic pwm load/etc), etc.
    
    * set channel address: given a serial number/chan number pair, assigns a logical address.  This address should be remembered by the device across resets.
    
    * query channel address: given serial number/chan number pair, device reports what address is currently assigned to that channel.
    
    * set channel values: given a collection of channel addresses and/or device/chan number pairs, and a corresponding collection of channel levels, set the specified channels to the specified levels.
    
    * query channel value: given a channel address or device/chan pair, get the current value of the specified channel

    * IR code received: when a device receives an IR code, it sends its device number and the code.