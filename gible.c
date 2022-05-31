#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "mmap.h"
#include "formats/ips.h"
#include "formats/ups.h"
#include "formats/bps.h"

#define HEADER_REGION_LEN 50

static void gible_main(int argc, char *argv[]);
static void gible_usage(const char *execname);

int main(int argc, char *argv[])
{
    gible_main(argc, argv);
    return 0;
}

static void gible_main(int argc, char *argv[])
{
    if (argc < 3)
        gible_usage(*argv);

    char *patchfile = argv[1];
    char *inputfile = argv[2];
    char *outputfile;

    if (argc < 4)
        outputfile = "output";
    else
        outputfile = argv[3];

    if (strcmp(patchfile, inputfile) == 0)
    {
        fprintf(stderr, "Error: Patch and input filenames are the same.\n");
        return;
    }

    if (strcmp(inputfile, outputfile) == 0)
    {
        fprintf(stderr, "Error: Input and output filenames are the same.\n");
        return;
    }

    if (strcmp(patchfile, outputfile) == 0)
    {
        fprintf(stderr, "Error: Patch and output filenames are the same.\n");
        return;
    }

    if (!file_exists(patchfile))
    {
        fprintf(stderr, "Patch file does not exist.\n");
        return;
    }

    if (!file_exists(inputfile))
    {
        fprintf(stderr, "Input file does not exist.\n");
        return;
    }

    uint8_t header_buf[HEADER_REGION_LEN];
    FILE *p = fopen(patchfile, "r");
    fread(header_buf, sizeof(char), HEADER_REGION_LEN, p);
    fclose(p);

    int ret = -1;
    if (ips_check(header_buf))
        ret = ips_patch_main(patchfile, inputfile, outputfile);
    if (ups_check(header_buf))
        ret = ups_patch_main(patchfile, inputfile, outputfile);
    if (bps_check(header_buf))
        ret = bps_patch_main(patchfile, inputfile, outputfile);

    if (ret == -1) fprintf(stderr, "Unsupported patch type.\n");
    else if (ret) fprintf(stderr, "Successfully patched files.\n");
    else fprintf(stderr, "Failed to patch.\n");

    return;
}

static void gible_usage(const char *execname)
{
    fprintf(
        stderr,
        "Usage:\n"
        "%s <patch> <input>\n"
        "%s <patch> <input> <output>\n",
        execname, execname);
    exit(EXIT_SUCCESS);
}
