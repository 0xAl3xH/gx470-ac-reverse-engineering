# GX470 AC Unit Reverse Engineering
This is an effort to reverse engineer the communication channel between the air conditioning system  and Multi function display on the Lexus GX470. Due to the tight integration between the AC control on this car and its antiquated headunit, many owners cannot upgrade to a more modern aftermarket headunit. This headunit system is often referred to as the "nav system/nav unit" in the GX community. Given its luxury SUV nature, most GX470s in the US were equipped with the nav unit. Owners of 2003-2009 GXes have limited options to upgrade to aftermarket android head units since many critircal AC controls require the old nav system's touch screen buttons. 

A workaround for this is finding a "non-nav conversion kit" which is essentially an AC control module from a lesser equipped GX470 without the nav unit. This unit ranges from rare to extremely rare depending on model year. Prices range from $250 on the cheaper end to $500. There are a few different part numbers for this kit and some years are not interchangeable.

This project aims to create a device that can act as the non-nav AC unit by decoding the AVC-LAN messages that are sent between the current air conditioning system and head unit. In more technical terms, we will perform aa man in the middle attack between the AC gateway ECU and stock nav head unit.

### Milestones

- Confirm Gateway ECU / Multi display junction speaks AVC-LAN
    - Program Arduino to use its internal comparators on differential signal
- Identify master/slave device addresses
    - HU address: 0x110<sub>16</sub> or 272<sub>10</sub>
    - Gateway/amplifier address: 0x1C6<sub>16</sub> or 454<sub>10</sub>

### Immediate tasks
- Improve Arduino AVC-LAN sniffer reliability
- Decode / Record all messages for all functionalities of the AC

### Future tasks
- Build driver circuit to send messsages via AVC LAN
    - See if A/C unit cares about master address

