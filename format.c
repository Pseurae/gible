#include "format.h"
#include "filemap.h"

int set_patch_error(patch_context_t *c, const char *err)
{
    c->error = err;
    return 0;
}