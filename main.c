#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#define PAGEMAP_BYTE_SIZE 8
#define PAGE_SIZE getpagesize() // default is 4096

typedef unsigned long u64;



void find_raddr(int pid, u64 start_vaddr, u64 end_vaddr, FILE *pagemap, FILE *db)
{
	int rc, i;
	u64 raddr, vaddr, offset;
	
	for (vaddr = start_vaddr; vaddr <= end_vaddr; vaddr += PAGE_SIZE) {
		
		offset = (vaddr / PAGE_SIZE) * PAGEMAP_BYTE_SIZE;

		rc = fseek(pagemap, offset, SEEK_SET);
		if (rc < 0)
			perror("fseek"), exit(1);

		fread(&raddr, PAGEMAP_BYTE_SIZE, 1, pagemap);

		raddr &= 0x007FFFFFFFFFFFFF; 

		/*
		 * If the vaddr is mapped to physical addr 0, we skip it.
		 */
		if (raddr == 0)
			continue;

		fprintf(db, "%d %llx   %llx\n", pid, vaddr, raddr);
	}

	return;
}



void read_maps(int pid, char *maps_buff, FILE *pagemap, FILE *db)
{
	u64 start_vaddr, end_vaddr;
	char *start, *end;
	start = maps_buff;
	
	fprintf(db, "PID     VADDR        RADDR\n");

	do {
		end = 0;
		end = strchr(start, '-');
		if (*end != '-')
			printf("Some unknown error occured\n"), exit(1);
		*end = '\0';

		start_vaddr = strtoull(start, NULL, 16);

		start = end + 1;

		end = 0;
		end = strchr(start, ' ');
		if (*end != ' ')
			printf("Some unknown error occured\n"), exit(1);
		*end = '\0';

		end_vaddr = strtoull(start, NULL, 16);

		find_raddr(pid, start_vaddr, end_vaddr, pagemap, db);

		while (*start != '\n')
			++start;

		++start;

	} while (*start != '\0');
}



int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s <PID>\n", argv[0]);
		exit(1);
	}

	int rc;

	char maps_path[32] = {0};
	char pagemap_path[32] = {0};

	snprintf(maps_path, sizeof(maps_path), "/proc/%s/maps", argv[1]);
	FILE *maps = fopen(maps_path, "rb");
	if (!maps) {
		perror("fopen(maps)");
		exit(1);
	}

	/*
	 * This is done to find the size of the file, so we can dynamically
	 * allocate the maps buffer.
	 *
	 * The first seek will move the pointer to the end of the file,
	 * then we use ftell to check the size.
	 * After this, we reset the pointer to the begenning by seeking it to
	 * the start.
	 */
	//fseek(maps, 0L, SEEK_END);
	//u64 maps_size = ftell(maps);
	//fseek(maps, 0L, SEEK_SET);
	//rewind(maps);
	
	u64 maps_size = 200000;
	char *maps_buff = calloc(maps_size, 1);
	rc = fread(maps_buff, sizeof(char), maps_size, maps);
	if (rc < 0)
		perror("fread"), exit(1);
	
	if (*maps_buff == '\0')
		printf("Maps not found!\n"), exit(1);

	snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%s/pagemap", argv[1]);
	FILE *pagemap = fopen(pagemap_path, "rb");
	if (!pagemap) {
		perror("fopen(pagemap)");
		exit(1);
	}

	
	FILE *db = fopen("./data", "a+");

	read_maps(atoi(argv[1]), maps_buff, pagemap, db);

	
	fclose(maps);
	fclose(pagemap);
	
	return 0;
}
