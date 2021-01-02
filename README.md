# NASAMarineDuetDriver
an nmea output for the NASAMarine Duett

Nasa Duett Integration
The four important signals can be picked up from the PIC micro and are, of course, 5V TTL 

The log itself provides two pulses per log wheel rotation (through a magnetic induction sensor and the amplification and pulse shaping in the instrument).
1Hz corresponds roughly to 0.2kts, this scales linearly. 
Therefore, expected frequencies are 1 to 40Hz or a minimum time between two pulses of 25ms

The depth sensor is a bit more involved as it has three signals:
* GATE (yellow)
* CHIRP (pink)
* ECHO (teal)

As far as I could tell, the CHIRP is sent as soon as GATE goes low, so my initial idea would be to use the falling edge of GATE as the start of measurement. 
Also, while the CHIRP is a signal of a few kHz, the ECHO seems to be just a pulse, so it’s rising edge can be used as the response.
The time from CHIRP to ECHO corresponds to the time the signal travelled through the water at ~1500m/s (speed of sound in salt water). For a 10cm resolution, that would require a measurement resolution of about 1333µs. The required range would be between 1m (=1.333ms) and 50m (66,667ms) which is a perfect fit for a 1MHz 16 Bit timer

Alternatively, a lower frequency could be used to save energy

References:
https://www.nmea.org/Assets/NMEA%200183%20Talker%20Identifier%20Mnemonics.pdf
https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:opencpn_user_manual:advanced_features:nmea_sentences&do=siteexport_addpage
