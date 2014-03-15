#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ubi-extractor.h"
#include "unubinize.h"
#include "ubifs-media.h"

char ubi_node_buffer[UBIFS_CH_SZ];
struct ubifs_ch* node_header;

int search_ubi_node_header(int verbose, int ubifs_fd)
{
	int found = 0;
	int eof = 0;
	int i = 0;
	while (!found && !eof)
	{
		if (readChar(ubifs_fd, 0x31, i, eof))
			if (readChar(ubifs_fd, 0x18, i, eof))
				if (readChar(ubifs_fd, 0x10, i, eof))
					if (readChar(ubifs_fd, 0x06, i, eof))
					{
						if (verbose)
							printf("Found magic\n");
						found++;
					}
	}

	if (found)
		return 1;

	return 0;
}

int read_ubi_node_header(int verbose, int ubifs_fd, unsigned long& node_len)
{
	if (lseek(ubifs_fd, -4, SEEK_CUR) == -1)
	{
		printf("Error: Seeking file!\n");
		return 0;
	}

	if (read(ubifs_fd, ubi_node_buffer, UBIFS_CH_SZ) != UBIFS_CH_SZ)
	{
		printf("Error: Reading ubi node header\n");
		return 0;
	}
	node_header = (struct ubifs_ch*)ubi_node_buffer;

	if (le32toh(node_header->magic) != UBIFS_NODE_MAGIC)
	{
		printf("Error: Node header no valid ubifs file. Missing UBIFS_NODE_MAGIC\n");
		return 0;
	}

	node_len = le32toh(node_header->len);

	if (verbose)
	{
		printf("Found Ubi node header\n");
		printf("  Magic %x\n", le32toh(node_header->magic));
		printf("  CRC %x\n", le32toh(node_header->crc));
		printf("  SeqNum %llu\n", le64toh(node_header->sqnum));
		printf("  Length %u\n", le32toh(node_header->len));
		printf("  Nodetype %d\n", node_header->node_type);
		printf("  Grouptype %d\n\n", node_header->group_type);
	}
	return 1;
}

int process_ubifs_volume(struct args args, int ubifs_fd)
{
	unsigned long node_len;
	while (search_ubi_node_header(args.verbose, ubifs_fd) == 1)
	{
		if (!read_ubi_node_header(args.verbose, ubifs_fd, node_len))
			continue;

		switch (node_header->node_type)
		{
			case UBIFS_INO_NODE: // inode node
				break;
			case UBIFS_DATA_NODE: // data node
			case UBIFS_DENT_NODE: // directory entry node
			case UBIFS_XENT_NODE: // extended attribute node
			case UBIFS_TRUN_NODE: // truncation node
			case UBIFS_PAD_NODE: // padding node
			case UBIFS_SB_NODE:  // superblock node
			case UBIFS_MST_NODE: // master node
			case UBIFS_REF_NODE: // LEB reference node
			case UBIFS_IDX_NODE: // index node
			case UBIFS_CS_NODE: // commit start node
			case UBIFS_ORPH_NODE: // orphan node
				break;
			default:
				printf("Warning: Unknown ubi node type\n");
				continue;
		}
	}

	return 1;
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
				if (lseek(args.out_tmp_fds[i], 0, SEEK_SET) == -1)
				{
					printf("Error seeking\n");
					continue;
					//TODO
				}
				process_ubifs_volume(args, args.out_tmp_fds[i]);
			}
		}
	}
	return 1;
}