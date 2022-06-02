#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "utils.h"
#include "mmap.h"
#include "format.h"
#include "formats/ips.h"
#include "formats/ups.h"
#include "formats/bps.h"

#define HEADER_REGION_LEN 50

static const patch_format_t *patch_formats[] = {
    &ips_patch_format,
    &ips32_patch_format,
    &ups_patch_format,
    &bps_patch_format
};

static void gible_main(int argc, char *argv[]);
static void gible_usage(const char *execname);
static int gible_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags);

int main(int argc, char *argv[])
{
    gible_main(argc, argv);
    return 0;
}

static void gible_main(int argc, char *argv[])
{
    if (argc < 4) {
        gible_usage(*argv);
        exit(EXIT_SUCCESS);
    }

    char *patchfile = argv[1];
    char *inputfile = argv[2];
    char *outputfile = argv[3];

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


    patch_flags_t flags;
    int ret = gible_patch(patchfile, inputfile, outputfile, &flags);

    if (ret == -1) fprintf(stderr, "Unsupported patch type.\n");

    return;
}

static void gible_usage(const char *execname)
{
    fprintf(
        stderr,
        "Usage:\n"
        "%s <patch> <input> <output>\n",
        execname);
}

static const char *mmap_errors[] = {
    "Cannot open the given patch file.",
    "Cannot open the given input file.",
    "Cannot open the given output file.",
};

static int gible_patch(char *pfn, char *ifn, char *ofn, patch_flags_t *flags)
{
    uint8_t header_buf[HEADER_REGION_LEN];
    FILE *p = fopen(pfn, "r");
    fread(header_buf, sizeof(char), HEADER_REGION_LEN, p);
    fclose(p);

    for (int i = 0; i < 4; i++) {
        const patch_format_t *format = patch_formats[i];

        if (format->check(header_buf)) { 
            int ret = format->main(pfn, ifn, ofn, flags);

            if (ret < ERROR_OUTPUT_FILE_MMAP)
                fprintf(stderr, "%s\n", format->error_msgs[ret]);
            else 
                fprintf(stderr, "%s\n", mmap_errors[INT16_MAX - ret - 1]);

            return ret;
        }
    }

    return -1;
}
