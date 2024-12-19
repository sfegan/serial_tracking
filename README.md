# serial_tracking

The `serial_tracking` program is the graphical user interface used to control the VERITAS motion control systems. 
It is used by the observers on duty to select targets, move and stop the telescopes, monitor their status, track 
objects in the sky, etc. It communicates with the VERITAS database and array control interface, and with the
commercial motion control systems in the pedestals of the telescopes.

It has a number of modes of functionment:

- Array GUI, allowing supervision of all for VERITAS telescopes from one interface using CORBA calls to the
  individual telescope controllers
- Telescope controller, interface between the array GUI and the motion control system, responsible for slewing
  to and tracking astronomical sources and monitoring the functioning of the telescopes to assure the safety
  of people and objects on the ground.
- Single telescope GUI, a fall-back GUI that can direct a single telescope through CORBA calls, or through
  an integrated telescope controller loop.
- Positioner emulator, to simulate the motion-control system and allow testing of the system.

## Array GUI ##

![Array interface](https://github.com/sfegab/serial_tracking/blob/main/VTracking/Doc/tucson_techtalk/array_gui.png?raw=true)
