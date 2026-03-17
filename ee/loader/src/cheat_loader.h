#ifndef CHEAT_LOADER_H
#define CHEAT_LOADER_H

#include <stdint.h>

// Maximum cheat list size (matches ee_core limits: 5 hooks + 250 codes, each 2 ints)
#define MAX_CHEATLIST_INTS ((5 * 2) + (250 * 2))
// Add 2 for the null terminator pair
#define CHEATLIST_BUF_SIZE (MAX_CHEATLIST_INTS + 2)

/*
 * Load cheats from one or more .cht files and build the CheatList int array.
 *
 * paths:     array of file path strings to .cht files
 * num_paths: number of paths in the array
 * dest:      pointer to output buffer (must hold at least CHEATLIST_BUF_SIZE ints)
 *
 * Returns the number of ints written (including the null terminator pair),
 * or 0 if no codes were loaded, or -1 on error.
 *
 * .cht file format (OPL/PS2RD compatible):
 *   - Lines starting with // or # are comments
 *   - Lines matching "XXXXXXXX YYYYYYYY" (hex pairs) are cheat codes
 *   - All other non-empty lines are cheat names (ignored)
 *   - Codes with address prefix 0x9XXXXXXX are hook/master codes
 */
int load_cheats_from_files(const char **paths, int num_paths, int *dest);

#endif
