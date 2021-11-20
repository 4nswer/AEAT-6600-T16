# AEAT-6600-T16
Test and config code for Broadcom AEAT-6600-T16 encoder using an Arduino Micro

This code is intended to provide a mechanism to test and program the Broadcom AEAT-6600-T16 Magnetic Rotary Encoder chip via the SSI pins using an Arduino Micro.

Due to the advantages of using direct port manipulation to achieve fast and stable SSI communication the pin assignment is hard coded.
As a result porting of the code to other microprocessors is non trivial and will require at minimum an understanding of direct port access.

See the circuit diagram for connection details. Pin assignment on the Micro is as follows:

D8 - MAG_HI/OTP_ERR

D9 - MAG_LO/OTP_PROG_STAT

D2 - ALIGN

D3 - PWRDOWN

D4 - PROG

D5 - NCS

D6 - SSI CLK

D7 - SSI DATA


The arduino listens for a character on the serial port. This changes the controller mode to one of the following:

'a' - Read position mode. Polls the position data every 200ms and posts the data to the serial port. Currently hardcoded to read 10 bits which is the chip default.
'b' - Magnetic intensity mode. Reports the state of the 2 magnetic intensity pins in the form of plain text messages.
'c' - Magnetic alignment mode. Changes the chip to alignment mode and polls the alignment value every 100ms and posts the data to the serial port.
'd' - Programming mode. Currently a work in progress.
'e' - Exits the selected mode.
