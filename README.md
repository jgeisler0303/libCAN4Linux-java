Right now this is a **lightly tested** bare-bones Java wrapper for the [CAN4Linux](http://sourceforge.net/projects/can4linux/) CAN bus driver. It only supports sending and receiving CAN messages. No baud-rate setting, no other driver config.

The code is heavily cribbed [libsocket-can-java](https://github.com/entropia/libsocket-can-java) and the `pyCan.c` example in the CAN4Linux source code.

I wrote the code to be able to use the integrated CAN bus of the [Banana Pi](http://www.lemaker.org/thread-13107-1-1.html) in Java, which relies on the can4linux driver; unlike the Raspberry Pi where there is a [SocketCAN](http://elinux.org/CAN_Bus) driver for an attached MCP2515 CAN bus controller chip.

Make sure you have the right version of 'can4linux.h' matching your installed driver in the 'include' directory!

To run the test got to 'bin.test' directory and run: 'java -cp .:../bin -Djava.library.path=../lib can4linux.CAN4LinuxTest'.
Make sure you have the CAN baud rate set up matching to your receiving end. Run some can bus monitor to see the message being sent by the test program. Then you have 10s to send some message that the test program will then display. If you get a mesage with identifier 0x7FFFFFFF you have an error in the transmission, most likely a baud rate mismatch. I will improve the test to display more detailed error messages later.

In the future I might add more functionality. But for now I will only test it and then move on to using it in my [openHAB CANopen binding](https://github.com/jgeisler0303/openHAB_CANopen_Binding).


