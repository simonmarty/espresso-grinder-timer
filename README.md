# Espresso Grinder Timer

My grinder (Rancilio Rocky) doesn't come with any dosage timing, just a push button to run the grinder. I added a relay in parallel to the push-button switch controlled by an Arduino as a timer.

This should work with just about any grinder as long as you've got 
- an Arduino (in my case it needs to output 5V to power the HD44780)
- a relay (either solid state or electromechanical)
- a rotary encoder (with a built-in switch in the stem, or a separate switch to start/stop the timer)
- I had an HD44780 I'm using to display the timer. You could use a 4 byte 7-segment dislay instead, you might just have to change the code a little.

The Arduino saves the timer in its EEPROM after 10 seconds without an update.

The display automatically dims after 5 seconds without user input.
