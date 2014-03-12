#include "unubinize.h"

struct args {
	unsigned int nounubi:1;
	unsigned int list_volumes:1;
	unsigned int extract:1;
	unsigned int show:1;
	unsigned int volume:1;
	char volumename[128];
	unsigned int inputfile:1;
	char inputfilename[1000];
	unsigned int outputdir:1;
	char outputdirname[1000];
	unsigned int verbose:1;
	unsigned int help:1;
};