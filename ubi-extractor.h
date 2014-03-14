#include "unubinize.h"

struct args {
	unsigned int nounubi:1;
	unsigned int list_volumes:1;
	unsigned int extract:1;
	unsigned int show:1;
	unsigned int volume:1;
	char input_volumename[128];
	unsigned int inputfile:1;
	char inputfilename[1000];
	unsigned int outputdir:1;
	char outputdirname[1000];
	unsigned int verbose:1;
	unsigned int help:1;
	int out_tmp_fds[128];
	char* out_filenames[128];
	char* ubi_volumenames[128];

	args()
	{
		int i;
		for (i = 0; i < 128; i++)
			out_tmp_fds[i] = -1;
	}
};