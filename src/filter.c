/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <utils/strutils.h>
#include <utils/utils.h>

#include "filter.h"

/* --- Filter handlers --- */

static int fluid_filter_strip(char *s, const char *arg1, const char *arg2)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);

    strip(s);
    return 0;
}

static int fluid_filter_lstrip(char *s, const char *arg1, const char *arg2)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);

    lstrip(s);
    return 0;
}

static int fluid_filter_rstrip(char *s, const char *arg1, const char *arg2)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);

    rstrip(s);
    return 0;
}

/* --- End of filter handlers --- */

/* Filter Flags */
#define LIQ_FF_NONE          0x00000000
#define LIQ_FF_HAS_1ARG      0x00000001
#define LIQ_FF_HAS_2ARGS     0x00000003

/**
 * TODO: `char *in` in handler should become fluid_type_t once we have our own
 * data type
 */
typedef struct {
    const char *identifier;
    int (*handler)(char *in, const char *arg1, const char *arg2);
    uint32_t flags;
} liq_filter_handler_t;

liq_filter_handler_t liq_filters[LIQ_FILTER_SENTINEL] = {
    [LIQ_FILTER_STRIP]       = { "strip",      fluid_filter_strip,      LIQ_FF_NONE },
    [LIQ_FILTER_LSTRIP]      = { "lstrip",     fluid_filter_lstrip,     LIQ_FF_NONE },
    [LIQ_FILTER_RSTRIP]      = { "rstrip",     fluid_filter_rstrip,     LIQ_FF_NONE },
};

enum liq_filter_e get_filter_id(const char *identifer)
{
    enum liq_filter_e i;

    for (i = LIQ_FILTER_NONE+1; i < LIQ_FILTER_SENTINEL; i++) {
        if(strcmp(identifer, liq_filters[i].identifier) == 0)
            break;
    }
    return i;
}

int liq_filter_arg_count(enum liq_filter_e id)
{
    uint32_t mask;

    if (id <= LIQ_FILTER_NONE || id >= LIQ_FILTER_SENTINEL)
        return -1;

    mask = (liq_filters[id].flags & LIQ_FF_HAS_2ARGS);
    if (mask == 0 || mask == 1)
        return mask;
    return 2;
}

int filter_execute(liq_filter_t *f, char *in)
{
    if (f->id <= LIQ_FILTER_NONE || f->id >= LIQ_FILTER_SENTINEL)
        return -1;

    return liq_filters[f->id].handler(in, f->args[0], f->args[1]);
}
