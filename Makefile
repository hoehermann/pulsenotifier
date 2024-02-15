all: pulsenotifier

pulsenotifier: pulsenotifier.c
	gcc -o pulsenotifier -I/usr/include/pulse/ pulsenotifier.c -lpulse -lhidapi-libusb

clean:
	rm pulsenotifier
