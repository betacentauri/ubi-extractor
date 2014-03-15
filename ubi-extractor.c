#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include "ubi-media.h"
#include "ubi-extractor.h"

struct args args;

const char version_string[] = "v0.1";

void printUsage()
{
	printf("Usage: ubi-extractor <parameter>\n");
	printf("Parameter:\n");
	printf("   -n --nounubi          use already unubinized input file\n");
	printf("   -l --listvolumes      list ubi volumes\n");
	printf("   -s --show             instead of extract print file and directory structure to stdout\n");
	printf("   -i file --input=x     input file\n");
	printf("   -o dir --output=x     output directory\n");
	printf("   -m volume --volume=x  use specific volume (e.g. -m rootfs)\n");
	printf("   -v --verbose          show more output\n");
	printf("   -h --help             show help\n");
}

int read_args(int argc, char *argv[])
{
	int option_index = 0;
	int opt;
	static const char *short_options = "nlsi:o:m:vh";
	static const struct option long_options[] = {
												{"nounubi"    , no_argument      , NULL, 'n'},
												{"listvolumes", no_argument      , NULL, 'l'},
												{"show"       , no_argument      , NULL, 's'},
												{"input"      , required_argument, NULL, 'i'},
												{"output"     , required_argument, NULL, 'o'},
												{"volume"     , required_argument, NULL, 'o'},
												{"verbose"    , no_argument      , NULL, 'v'},
												{"help"       , no_argument      , NULL, 'h'},
												{NULL         , no_argument      , NULL,  0} };

	while ((opt= getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
	{
		switch (opt)
		{
			case 'n':
				args.nounubi = 1;
				break;
			case 'l':
				args.list_volumes = 1;
				break;
			case 's':
				args.show = 1;
				break;
			case 'i':
				if (optarg)
				{
					if (strlen(optarg) <= 999)
					{
						printf("Input file %s\n", optarg);
						strcpy(args.inputfilename, optarg);
						args.inputfile = 1;
					}
				}
				break;
			case 'o':
				if (optarg)
				{
					if (strlen(optarg) <= 999)
					{
						printf("Output directory %s\n", optarg);
						strcpy(args.outputdirname, optarg);
						args.outputdir = 1;
					}
				}
				break;
			case 'm':
				if (optarg)
				{
					if (strlen(optarg) <= 128)
					{
						printf("Process volume %s\n", optarg);
						strcpy(args.input_volumename, optarg);
						args.volume = 1;
					}
				}
				break;
			case 'v':
				args.verbose = 1;
				break;
			case '?':
			case 'h':
				args.help = 1;
				return 0;
		}
	}

	if (argc == 1)
	{
		args.help = 1;
		return 0;
	}
	if (args.inputfile == 0)
	{
		printf("Error: Missing input file\n\n");
		args.help = 1;
		return 0;
	}
	if (args.outputdir == 0)
	{
		printf("Warning: Using ./ as output directory\n");
		strcpy(args.outputdirname, "./");
		args.outputdir = 1;
	}

	return 1;
}

int open_file(int& fd, char* filename)
{
	fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		printf("Error: Could not open file %s\n", filename);
		return 0;
	}
	return 1;
}

int main(int argc, char *argv[])
{
	printf("Ubi extractor version %s\n", version_string);
	printf("Author: Betacentauri\n\n");

	int ret;
	ret = read_args(argc, argv);
	if (!ret || args.help)
	{
		printUsage();
		return -1;
	}

	printf("\n");

	if (!args.nounubi)
	{
		if (!unubinize(&args))
		{
			return 1;
		}
	}

	ubifs_reader(args);

	return 0;
}
