SRC = ubi-extractor.c unubinize.c crc32.c ubifs_reader.c

OBJ = $(SRC:.c=.o)

OUT = ubi-extractor

LDFLAGS= -static

CFLAGS = -O2

CC = g++

.SUFFIXES: .cpp

default: $(OUT_LIB) $(OUT)

.cpp.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ) 
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(OUT)
