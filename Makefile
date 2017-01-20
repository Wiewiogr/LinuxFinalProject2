COMMON_FLAGS = -Wl,-rpath=./ -L./ -lCommon

all: archiwista.out brygadzista.out robotnik.out

brygadzista.out: brygadzista.c libCommon.so
	gcc -o brygadzista.out brygadzista.c $(COMMON_FLAGS)

robotnik.out: robotnik.c libCommon.so
	gcc -o robotnik.out robotnik.c $(COMMON_FLAGS)

archiwista.out: archiwista.c libCommon.so
	gcc -o archiwista.out archiwista.c $(COMMON_FLAGS)

libCommon.so : common.c
		gcc -fPIC -shared -o libCommon.so common.c -lm -lrt

clean:
	rm *.out *.so
