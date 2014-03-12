#include <stdio.h>
#include <stdlib.h>
#include <map>
#include "ubi-media.h"
#include "ubi-extractor.h"

FILE* ubifs_file;
unsigned long peb_size = 0;
unsigned long vid_hdr_offset = 0;
unsigned long data_offset = 0;
unsigned long data_pad = 0;
struct ubi_vid_hdr* vid_hdr;
char vid_hdr_buf[UBI_VID_HDR_SIZE];
typedef std::map<unsigned long, unsigned long long> t_seqn_map;
typedef std::map<unsigned long, t_seqn_map> t_vol_seqn_map;

int open_file(char* filename)
{
	ubifs_file = fopen(filename, "r");
	if (ubifs_file == NULL)
	{
		printf("Error: Could not open file %s\n", filename);
		return 0;
	}
	return 1;
}

int check_ec_header(char* buffer, int verbose)
{
	struct ubi_ec_hdr* ec_hdr;

	ec_hdr = (struct ubi_ec_hdr*) buffer;
	if (be32toh(ec_hdr->magic) != UBI_EC_HDR_MAGIC)
	{
		printf("Error: EC header no valid ubifs file. Missing UBI_EC_HDR_MAGIC\n");
		return 0;
	}

	if (ec_hdr->version != 1)
	{
		printf("Error: EC header Unknown ubifs version %u\n", ec_hdr->version);
		return 0;
	}

	if (mtd_crc32(UBI_CRC32_INIT, ec_hdr, UBI_EC_HDR_SIZE_CRC) != be32toh(ec_hdr->hdr_crc))
	{
		printf("Error: EC header CRC checksum error\n");
		return 0;
	}

	if (vid_hdr_offset == 0)
	{
		vid_hdr_offset = be32toh(ec_hdr->vid_hdr_offset);
		data_offset = be32toh(ec_hdr->data_offset);
	}

	if (verbose)
	{
		printf("UBI Erase counter header:\n");
		printf("  Magic: %x\n", be32toh(ec_hdr->magic));
		printf("  Version: %x\n", ec_hdr->version);
		printf("  Erase Counter: %llu\n", be64toh(ec_hdr->ec));
		printf("  vid_hdr_offset: %x\n", be32toh(ec_hdr->vid_hdr_offset));
		printf("  data_offset: %x\n", be32toh(ec_hdr->data_offset));
		printf("  image_seq: %x\n", be32toh(ec_hdr->image_seq));
		printf("  crc: %x\n", be32toh(ec_hdr->hdr_crc));
	}

	return 1;
}

int read_ubigen_vol_info()
{
	struct ubigen_vol_info* vi;
	
}

int check_vid_header(char* buffer, int verbose)
{
	vid_hdr = (struct ubi_vid_hdr*) (buffer + vid_hdr_offset);

	if (be32toh(vid_hdr->magic) != UBI_VID_HDR_MAGIC)
	{
		return 0;
	}

	if (vid_hdr->version != 1)
	{
		printf("Error: Vid header unknown ubifs version %u\n", vid_hdr->version);
		return 0;
	}

	if (mtd_crc32(UBI_CRC32_INIT, vid_hdr, UBI_VID_HDR_SIZE_CRC) != be32toh(vid_hdr->hdr_crc))
	{
		printf("Error: Vid header CRC checksum error\n");
		return 0;
	}

	if (vid_hdr->vol_type == 2) // static volume -> data crc present
	{
		if (mtd_crc32(UBI_CRC32_INIT, buffer + data_offset, peb_size - data_offset) != be32toh(vid_hdr->data_crc))
		{
			printf("Error: Vid header data CRC checksum error\n");
			return 0;
		}
	}

	data_pad = be32toh(vid_hdr->data_pad);
	if (verbose)
	{
		printf("UBI volume identifier header:\n");
		printf("  Magic: %x\n", be32toh(vid_hdr->magic));
		printf("  Version: %x\n", vid_hdr->version);
		printf("  Vol_type: %x\n", vid_hdr->vol_type);
		printf("  copy_flag: %x\n", vid_hdr->copy_flag);
		printf("  Compat: %x\n", vid_hdr->compat);
		printf("  vol_id: %x\n", be32toh(vid_hdr->vol_id));
		printf("  lnum: %x\n", be32toh(vid_hdr->lnum));
		printf("  data_size: %x\n", be32toh(vid_hdr->data_size));
		printf("  used_ebs: %x\n", be32toh(vid_hdr->used_ebs));
		printf("  data_pad: %x\n", be32toh(vid_hdr->data_pad));
		printf("  data_crc: %x\n", be32toh(vid_hdr->data_crc));
		printf("  sqnum: %llu\n", be64toh(vid_hdr->sqnum));
		printf("  crc: %x\n", be32toh(vid_hdr->hdr_crc));
	}

	return 1;
}

int readChar(unsigned int ch, int *i)
{
	unsigned int b;
	++*i;
	if ((b = (unsigned int)fgetc(ubifs_file)) == EOF)
		return 0;

	if (b == ch)
		return 1;
	return 0;
}

int guess_peb_size(int verbose)
{
	int i = data_offset;
	const int max_bytes = 1024*1024*30;
	int found = 0;
	int pebs[4];

	if (fseek(ubifs_file, data_offset, SEEK_SET) != 0)
	{
		printf("Error: File too small!\n");
		return 0;
	}

	while (found < 4 && i < max_bytes)
	{
		if (readChar(0x55, &i)) // U
			if (readChar(0x42, &i))  // B
				if (readChar(0x49, &i)) // I
					if (readChar(0x23, &i)) // #
					{
						if (verbose)
							printf("Found peb_size %d, found %d\n", i - 4, found);
						pebs[found] = i - 4;
						found++;
					}
	}

	if (found == 4)
	{
		pebs[3] = pebs[3]-pebs[2];
		pebs[2] = pebs[2]-pebs[1];
		pebs[1] = pebs[1]-pebs[0];
		if (pebs[0] == pebs[1] && pebs[1] == pebs[2] && pebs[2] == pebs[3])
		{
			peb_size = pebs[0];
			printf("Using PEB size: %lu\n", peb_size);
			return 1;
		}
		else
			printf ("Error: Found different PEB sizes: %d %d %d %d -> abort\n", pebs[0], pebs[1], pebs[2], pebs[3]);
	}
	return 0;
}

int read_volume_table(char* buffer)
{
	int i;
	struct ubi_vtbl_record* vtbl_rec;
	printf("\nFound volumes:\n");
	for (i = 0; i < peb_size/UBI_VTBL_RECORD_SIZE; i++)
	{
		if (i == UBI_MAX_VOLUMES-1)
			break;

		vtbl_rec  = (struct ubi_vtbl_record*) (buffer + i * UBI_VTBL_RECORD_SIZE);
		if (be32toh(vtbl_rec->reserved_pebs) == 0)
			continue;

		if (mtd_crc32(UBI_CRC32_INIT, vtbl_rec, UBI_VTBL_RECORD_SIZE_CRC) != be32toh(vtbl_rec->crc))
		{
			printf("Error: Volume table CRC checksum error\n");
			return 0;
		}

		printf("ID %d: %s\n", i, vtbl_rec->name);
		if (vtbl_rec->upd_marker)
			printf("  Update marker set -> Volume corrupt\n");
	}
	return 1;
}

int process_file(struct args args)
{
	int i = 0;
	int first_vol_tab = 1;
	char* buffer = new char[peb_size];
	FILE* output_file;
	unsigned long vol_id;
	unsigned long logical_eb_number;
	unsigned long long seq_number;
	unsigned long long ret_val;
	t_vol_seqn_map vol_seq_map;

	if (fseek(ubifs_file, 0, SEEK_SET) != 0)
	{
		printf("Error: Seeking file failed\n");
		goto err;
	}

	output_file = fopen("ubi.out", "w");
	if (output_file == NULL)
	{
		printf("Could not open file: %s\n", "ubi.out");
		goto err;
	}

	while (fread(buffer, peb_size, 1, ubifs_file) == 1)
	{
		if (!check_ec_header(buffer, args.verbose))
			goto err;

		if (!check_vid_header(buffer, args.verbose))
			continue;  // unused erase blocks have no volume identifier header

		if (be32toh(vid_hdr->vol_id) == UBI_INTERNAL_VOL_START)
		{
			if (args.list_volumes && first_vol_tab == 1)
			{
				if (read_volume_table(buffer + data_offset))
					first_vol_tab++;
			}
		}
		else
		{
			vol_id = be32toh(vid_hdr->vol_id);
			logical_eb_number = be32toh(vid_hdr->lnum);
			seq_number = be64toh(vid_hdr->sqnum);
			ret_val = vol_seq_map[vol_id][logical_eb_number];
			if (seq_number <= ret_val && ret_val != 0)
				continue; // same logical erase block but lower seq_number -> old erase block

			vol_seq_map[vol_id][logical_eb_number] = seq_number;
			if (fseek(output_file, (peb_size - data_offset - data_pad) * logical_eb_number, SEEK_SET) != 0)
			{
				printf("Error seeking\n");
				goto err;
			}
			if (fwrite(buffer + data_offset, peb_size - data_offset - data_pad, 1, output_file) != 1)
			{
				printf("Error: Writing ubi file\n");
				goto err;
			}
		}
		i++;
	}

	if (feof(ubifs_file))
	{
		fclose(output_file);
		delete  buffer;
		return 1;
	}

	printf("Error: File size is not multiple of peb size!\n");

err:
	if (output_file)
		fclose(output_file);
	if (buffer)
		delete  buffer;
	return 0;
}

int unubinize(struct args args)
{
	printf("Starting unubinize\n");
	char ec_hdr_buf[UBI_EC_HDR_SIZE];

	if (!open_file(args.inputfilename))
		goto err;

	if (fread(&ec_hdr_buf, UBI_EC_HDR_SIZE, 1, ubifs_file) != 1)
	{
		printf("Error: fread cannot read file\n");
		return 0;
	}
	if (!check_ec_header(ec_hdr_buf, args.verbose))
		goto err;

	if (!guess_peb_size(args.verbose))
		goto err;

	if (!process_file(args))
		goto err;

	fclose(ubifs_file);
	return 1;
err:
	if (ubifs_file)
		fclose(ubifs_file);
	return 0;
}
