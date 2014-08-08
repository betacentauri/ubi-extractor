#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ubi-extractor.h"
#include "unubinize.h"
#include "ubifs-media.h"
#include "key.h"

char ubi_node_header_buffer[UBIFS_CH_SZ];
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

	if (read(ubifs_fd, ubi_node_header_buffer, UBIFS_CH_SZ) != UBIFS_CH_SZ)
	{
		printf("Error: Reading ubi node header\n");
		return 0;
	}
	node_header = (struct ubifs_ch*)ubi_node_header_buffer;

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

int read_ubi_node(char** node_buffer, int ubifs_fd, unsigned long node_len)
{
	*node_buffer = new char[node_len];
	if (read(ubifs_fd, *node_buffer + UBIFS_CH_SZ, node_len - UBIFS_CH_SZ) != (node_len - UBIFS_CH_SZ))
	{
		printf("Error: Reading ubi node. Node is smaller than specified length in the header\n");
		delete[] *node_buffer;
		return 0;
	}
	return 1;
}

int process_inode_node(int verbose, int ubifs_fd, unsigned long node_len)
{
	char* node_buffer = NULL;
	if (!read_ubi_node(&node_buffer, ubifs_fd, node_len))
		return 0;

	struct ubifs_ino_node* node = (struct ubifs_ino_node*)node_buffer;
	printf("key inum %lu\n", key_inum_flash(NULL, node->key));
	printf("key type %d\n", key_type_flash(NULL, node->key));
	printf("Create_SeqNum %llu\n", le64toh(node->creat_sqnum));
	printf("Size %llu\n\n", le64toh(node->size));
	delete[] node_buffer;
	return 1;
}

int process_dir_node(int verbose, int ubifs_fd, unsigned long node_len)
{
	char* node_buffer = NULL;
	if (!read_ubi_node(&node_buffer, ubifs_fd, node_len))
		return 0;

	struct ubifs_dent_node* node = (struct ubifs_dent_node*)node_buffer;
	printf("key inum %lu\n", key_inum_flash(NULL, node->key));
	printf("key type %d\n", key_type_flash(NULL, node->key));
	printf("inum %llu\n", le64toh(node->inum));
	printf("type %x\n", node->type);
	printf("name %s\n\n", node->name);
	delete[] node_buffer;
	return 1;
}

int process_index_node(int verbose, int ubifs_fd, unsigned long node_len)
{
	char* node_buffer = NULL;
	if (!read_ubi_node(&node_buffer, ubifs_fd, node_len))
		return 0;

	struct ubifs_idx_node* node = (struct ubifs_idx_node*)node_buffer;
	printf("child_cnt %u\n", le16toh(node->child_cnt));
	printf("level %u\n", le16toh(node->level));
	printf("branches:\n");
	int i;
	ubifs_branch* b;
	for (i = 0; i < le16toh(node->child_cnt); i++)
	{
		b = (ubifs_branch*)(node->branches + i * (UBIFS_BRANCH_SZ + UBIFS_SK_LEN));
		printf("  i: %d lnum %u\n", i, le32toh(b->lnum));
		printf("  i: %d off %u\n", i, le32toh(b->offs));
		printf("  i: %d len %u\n", i, le32toh(b->len));
		printf("  i: %d key inum %lu\n", i, key_inum_flash(NULL, b->key));
		printf("  i: %d key type %d\n", i, key_type_flash(NULL, b->key));
	}
	delete[] node_buffer;
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
				printf("inode node\n");
				process_inode_node(args.verbose, ubifs_fd, node_len);
				break;
			case UBIFS_DATA_NODE: // data node
				printf("data node\n");
				break;
			case UBIFS_DENT_NODE: // directory entry node
				printf("directory entry node node\n");
				process_dir_node(args.verbose, ubifs_fd, node_len);
				break;
			case UBIFS_XENT_NODE: // extended attribute node
				printf("extended attribute node\n");
				break;
			case UBIFS_TRUN_NODE: // truncation node
				break;
			case UBIFS_PAD_NODE: // padding node
				break;
			case UBIFS_SB_NODE:  // superblock node
				printf("superblock node\n");
				break;
			case UBIFS_MST_NODE: // master node
				printf("master node\n");
				break;
			case UBIFS_REF_NODE: // LEB reference node
				printf("LEB reference node\n");
				break;
			case UBIFS_IDX_NODE: // index node
				printf("index node\n");
				process_index_node(args.verbose, ubifs_fd, node_len);
				break;
			case UBIFS_CS_NODE: // commit start node
				printf("commit start node\n");
				break;
			case UBIFS_ORPH_NODE: // orphan node
				printf("orphan node\n");
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