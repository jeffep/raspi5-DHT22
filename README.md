# raspi5-DHT22
C++ code for connecting a DHT22 temperature sensor to the Raspberry Pi 5

Apparently the libraries for accessing GPIO, wiringpi is being deprecated.
I used gpiod library instead.  I downloaded v2.3 off github and used that
in /usr/include (instead of /usr/local/include where apt get put it's latest).

All my AI helpers suggested using interrupts to watch for and wait until
3.3v (1) was detected and to read the data that way.  I could not get
that to work.

Instead when the PI sends the wakeup signal, I turn the GPIO to input
and immediately start sampling the line.  On the WIZNET Surf 7500 board
even this was too slow and I had to overclock the board by a factor of 4.
This would result in 5 samplings of 3.3v for a '0' bit.  Fortunately the
Raspberry PI is much faster and reads roughly 21 samples for a '0'.  Read
all this data into an array and then it's easily deconstructed into the
good data.

The executable has -h for help.  Default is to use 3.3v, ground, and PIN7 = GPIO4 for
the DHT connections.  I didn't have the recommended 4.7k Ohm resistor, but I had a 10k
lying around so I used that and it worked.  But I never used a resistor on the PICOs.

![image](https://github.com/user-attachments/assets/f5854292-9264-40d1-b415-f4fd901e9641)


Compile statement: g++ -o dht22 src/main.cpp src/DHT22.cpp -Iinclude -I/usr/include -L/usr/lib -lgpiod

