/* EXT2 DRIVER 
 * EXT2 File system driver
 * 7/11/26 MaXOS -- ext2 driver main file 
*/
#include "ext2.h"
#include "../../drivers/blk/atapio.h"
#include "../../printf.h"
#include "../../string.h"

static ext2_superblock_t sb;

void ext2_init(uint8_t drive) {
	uint8_t buf[1024];
	if (ata_read_sector(drive,2,1,buf)!=0) {
		kprintf("Err, failed to read superblock.\n");
		return;
	}
	memcpy(&sb, buf, sizeof(ext2_superblock_t));
	if (sb.s_magic != EXT2_SUPER_MAGIC) {
		kprintf("Invalid filesystem\nWhatever, using initramfs\n");
		return;
	}
	// for debug
	//kprintf("Valid superblock found.\n");
	//kprintf("Inodes: %u|Blocks: %u\n", sb.s_inodes_count, sb.s_blocks_count);
	//kprintf("Block size: %ubytes\n", 1024 << sb.s_log_block_size);
}

int ext2_scan_all(void) {
    int found = 0;
    
    for (int drive = 0; drive < ATA_MAX_DRIVES; drive++) {
        if (!ata_is_drv_presnt(drive)) continue;
        
        uint8_t buf[1024];
        if (ata_read_sector(drive, 2, 1, buf) != 0) {
            kprintf("Drive %d: Read error \t\t\t\t\t\t\t \033[33mERR\033[0m\n\n", drive);
            continue;
        }
        
        ext2_superblock_t *sb = (ext2_superblock_t *)buf;
        if (sb->s_magic == EXT2_SUPER_MAGIC) {
            kprintf("drive %d \t\t\t\t\t\t\t \033[32mOK\033[0m\n", drive);
            found++;
        } else {
            kprintf("Drive %d not ext2\t\t\t\t\t\t \033[33mWRN\033[0m\n", drive, sb->s_magic);
        }
    }
    
    if (found == 0) {
    	;
    } else {
        kprintf("Found %d ext2 filesystem(s).\n", found);
    }
    
    return found;
}
