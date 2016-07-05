This is a simple set of receivers for digital voice modes based on the DV4mini stick.

The first receiver, for System Fusion, dumps all of the information in a System Fusion transmission except for the audio.

Usage is: YSFRX serial_port freq_in_hertz

The second receiver, for DMR, is only for monitoring the output of DMR repeaters. Like the System Fusion receiver, it decodes control information, not the audio.

Usage is: DMRRX serial_port freq_in_hertz

The third receiver, for D-Star dumps all of the information in a D-Star transmission except for the audio. The slow data is not interpreted, but merely displayed as hex and characters.

Usage is: DStarRX serial_port freq_in_hertz
