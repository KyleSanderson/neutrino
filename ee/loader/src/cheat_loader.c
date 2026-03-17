/*
 * Cheat file loader for Neutrino
 *
 * Reads OPL/PS2RD-compatible .cht files and builds the CheatList
 * integer array consumed by ee_core's cheat engine.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cheat_loader.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Static read buffer: must be cache-line aligned (64 bytes) for PS2 fileXio SIF DMA.
 * A stack-allocated buffer causes DMA to overwrite adjacent stack variables.
 */
static char _cht_read_buf[4096] __attribute__((aligned(64)));

// Check if a character is a hex digit
static int is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// Parse an 8-character hex string into a uint32_t. Returns 0 on success, -1 on error.
static int parse_hex8(const char *s, unsigned int *out)
{
    unsigned int val = 0;
    int i;
    for (i = 0; i < 8; i++) {
        char c = s[i];
        val <<= 4;
        if (c >= '0' && c <= '9')
            val |= c - '0';
        else if (c >= 'a' && c <= 'f')
            val |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            val |= c - 'A' + 10;
        else
            return -1;
    }
    *out = val;
    return 0;
}

/*
 * Try to parse a line as a cheat code: "XXXXXXXX YYYYYYYY"
 * Returns 1 if parsed successfully, 0 otherwise.
 */
static int parse_code_line(const char *line, unsigned int *addr, unsigned int *val)
{
    int i;

    // Skip leading whitespace
    while (*line && isspace((unsigned char)*line))
        line++;

    // Need at least 17 characters: 8 hex + space + 8 hex
    for (i = 0; i < 8; i++) {
        if (!is_hex(line[i]))
            return 0;
    }

    if (line[8] != ' ' && line[8] != '\t')
        return 0;

    for (i = 9; i < 17; i++) {
        if (!is_hex(line[i]))
            return 0;
    }

    // Verify line ends here (possibly with trailing whitespace/newline)
    for (i = 17; line[i]; i++) {
        if (!isspace((unsigned char)line[i]))
            return 0;
    }

    if (parse_hex8(line, addr) < 0)
        return 0;
    if (parse_hex8(line + 9, val) < 0)
        return 0;

    return 1;
}

/*
 * Load cheats from a single file, appending to dest starting at *offset.
 * Returns 0 on success, -1 on error.
 */
static int load_single_cht(const char *path, int *dest, int *offset)
{
    int fd;
    char *buf = _cht_read_buf;
    char line[256];
    int line_pos = 0;
    int bytes_read;
    int i;
    unsigned int addr, val;

    memset(line, 0, sizeof(line));

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("WARN: Unable to open cheat file %s\n", path);
        return -1;
    }

    printf("Loading cheats from %s\n", path);

    while ((bytes_read = read(fd, buf, sizeof(_cht_read_buf))) > 0) {
        for (i = 0; i < bytes_read; i++) {
            char c = buf[i];

            if (c == '\n' || c == '\r') {
                line[line_pos] = '\0';

                // Skip empty lines
                if (line_pos == 0)
                    goto next_char;

                // Skip comment lines
                if (line[0] == '#')
                    goto next_char;
                if (line[0] == '/' && line[1] == '/')
                    goto next_char;

                // Try to parse as code
                if (parse_code_line(line, &addr, &val)) {
                    if (*offset < MAX_CHEATLIST_INTS) {
                        dest[*offset] = (int)addr;
                        dest[*offset + 1] = (int)val;
                        *offset += 2;
                    } else {
                        printf("WARN: Cheat list full, skipping remaining codes\n");
                        close(fd);
                        return 0;
                    }
                }
                // else: cheat name line, ignore

            next_char:
                line_pos = 0;
                continue;
            }

            if (line_pos < (int)sizeof(line) - 1)
                line[line_pos++] = c;
        }
    }

    // Handle last line if file doesn't end with newline
    if (line_pos > 0) {
        line[line_pos] = '\0';
        if (line[0] != '#' && !(line[0] == '/' && line[1] == '/')) {
            if (parse_code_line(line, &addr, &val)) {
                if (*offset < MAX_CHEATLIST_INTS) {
                    dest[*offset] = (int)addr;
                    dest[*offset + 1] = (int)val;
                    *offset += 2;
                }
            }
        }
    }

    close(fd);
    return 0;
}

int load_cheats_from_files(const char **paths, int num_paths, int *dest)
{
    int offset = 0;
    int i;
    int any_loaded = 0;

    // Clear buffer
    memset(dest, 0, CHEATLIST_BUF_SIZE * sizeof(int));

    for (i = 0; i < num_paths; i++) {
        if (load_single_cht(paths[i], dest, &offset) == 0)
            any_loaded = 1;
    }

    if (!any_loaded || offset == 0)
        return 0;

    // Null terminator pair
    dest[offset] = 0;
    dest[offset + 1] = 0;
    offset += 2;

    printf("Loaded %d cheat code pairs from %d file(s)\n", (offset - 2) / 2, num_paths);
    return offset;
}
