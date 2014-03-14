#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ubi-extractor.h"


int process_ubifs_volume(struct args args, int ubifs_fd)
{
	
}

int ubifs_reader(struct args args)
{
	int fd;
	printf("Reading Ubifs\n");

	if (args.nounubi)
	{
		if (!open_file(fd, args.inputfilename))
			return 0;

		process_ubifs_volume(args, fd);
	}
	else
	{
		int i;
		for (i = 0; i < 128; i++)
		{
			if (args.volume && args.ubi_volumenames[i])
				if (strcmp(args.input_volumename, args.ubi_volumenames[i]) == 0)
				{
					printf("Processing volume %s\n", args.ubi_volumenames[i]);
					process_ubifs_volume(args, args.out_tmp_fds[i]);
				}
				else
					continue;
			else if (args.ubi_volumenames[i])
			{
				printf("Processing volume %s\n", args.ubi_volumenames[i]);
				process_ubifs_volume(args, args.out_tmp_fds[i]);
			}
		}
	}
}