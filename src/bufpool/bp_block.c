/*
 * bp_block.c — Buffer pool block (header-only value type; no body needed)
 *
 * The bp_block_t struct is defined entirely in bp_block.h.  This
 * translation unit exists to satisfy linker archives and to provide
 * a natural home for any future block-level helpers.
 */

#include "bp_block.h"

/* No standalone functions required for the block value type. */
