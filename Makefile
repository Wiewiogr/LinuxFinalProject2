all: archiwista.out brygadzista.out robotnik.out

brygadzista.out: brygadzista.c
	gcc -o brygadzista.out brygadzista.c

robotnik.out: robotnik.c
	gcc -o robotnik.out robotnik.c

archiwista.out: archiwista.c
	gcc -o archiwista.out archiwista.c

clean:
	rm *.out
