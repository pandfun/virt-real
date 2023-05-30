#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>


/*
 * This value is used to create the 2d array 
 * to hold the virtual start and end addr from
 * /proc/PID/maps
 *
 * The number of entries may vary based on the process,
 * hence this value is just an estimate.
 */
#define VIRT_ENTRIES 2000

#define MAPS_BUFF 200000


#define PAGEMAP_BYTE_SIZE 8
#define PAGE_SIZE getpagesize() // 4096

typedef unsigned long U64;

U64 vaddr_buff[VIRT_ENTRIES][2]; // Array to hold the virt addr


/*
 * Function to add the values of start and end virtual address
 * in the "vaddr_buff" array.
 */
U64 parse_vaddr(char *buff)
{
	U64 ventry_index = 0;
	
	char *start, *end;
	start = buff;

	do {
		end = strchr(start, '-');
		*end = '\0';
		vaddr_buff[ventry_index][0] = strtoull(start, NULL, 16);
		
		start = end + 1;

		end = strchr(start, ' ');
		*end = '\0';
		vaddr_buff[ventry_index][1] = strtoull(start, NULL, 16);
		
		
		while (*start != '\n') {
			++start;
		}

		++start;
		++ventry_index;
		
	} while (*start != '\0');

	return ventry_index;
}


/*
 * Function to find & print real addr for all the virtual addr in 
 * the "vaddr_buff" array.
 * 
 * TODO: Format output
 */
void read_pagemaps(int pid, char *path, U64 ventries)
{
	FILE *pagemap = fopen(path, "rb");
	if (!pagemap)
		perror("fopen"), exit(1);

	int ret, i;

	U64 raddr, vaddr, offset;

	printf("PID\t    VIRTUAL ADDR  \t\t\t  REAL ADDR\n");

	for (i = 0; i < ventries; ++i) {
		
		vaddr = vaddr_buff[i][0];
		for (; vaddr < vaddr_buff[i][1]; vaddr += PAGE_SIZE) {
			
			offset = (vaddr / PAGE_SIZE) * PAGEMAP_BYTE_SIZE;

			ret = fseek(pagemap, offset, SEEK_SET);
			if (ret < 0)
				perror("fseek"), exit(1);

			fread(&raddr, PAGEMAP_BYTE_SIZE, 1, pagemap);

			printf("%d\t%llx (%llu)\t\t%llx (%llu)\n", pid, vaddr, vaddr, raddr, raddr);
		}
	}
	
	fclose(pagemap);
	return;
}


int main(int argc, char *argv[])
{
	if (argc != 2)
		printf("Usage: %s <PID>", argv[0]), exit(1);

	
	int rc;

	char maps_path[32] = {0};
	char pagemap_path[32] = {0};


	snprintf(maps_path, sizeof(maps_path), "/proc/%s/maps", argv[1]);
	FILE *maps = fopen(maps_path, "rb");
	if (!maps)
		perror("fopen"), exit(1);

	char maps_buff[MAPS_BUFF] = {0};
	rc = fread(maps_buff, sizeof(char), MAPS_BUFF, maps);

	U64 ventries = parse_vaddr(maps_buff);

	
	snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%s/pagemap", argv[1]);
	read_pagemaps(atoi(argv[1]), pagemap_path, ventries);
	
	fclose(maps);
	return 0;
}
