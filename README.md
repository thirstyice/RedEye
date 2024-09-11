# RedEye
### A library implementing the "RedEye" infrared protocol
===

The RedEye protocol was mostly used by HP graphing calculators to connect to each other or to a small printer. It is a precursor to the now much more popular IrDA infrared protocol.

This library implements the byte-level protocol, but not HP's character encodings, graphics mode(s), etc.

I made this library mostly so I could get digital readouts from JSI dimmer retrofit kits (rather than having to deal with 5-10 meters of receipt paper from the official printer), but as many of those are now being replaced with newer models, my access to official hardware that uses this protocol is dwindling (at least until I decide to pick up an old hp calculator off ebay lol).

#### Note:
Only works with clock speed >= 16MHz
12MHz may also work, but YMMV