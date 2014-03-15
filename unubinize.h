#include <stdint.h>
#include "crc32.h"

int unubinize(struct args* args);
int readChar(int fd, unsigned int ch, int &i, int &eof);