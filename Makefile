.PHONY: all clean

all: b64encode extrac4

clean: 
	rm -f *.o b64encode extrac4

b64encode: b64encode.c base64.c

extrac4: extrac4.cpp base64.c crc.c

