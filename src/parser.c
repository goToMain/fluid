/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>

#include "parser.h"
#include "lexer.h"
#include "liquid.h"

#define MAX_BLOCK_NESTING_DEPTH        64

void parser_setup(fluid_t *ctx)
{
    ARG_UNUSED(ctx);
}

/* last - Liquid Abstract Syntax Tree */
struct last_s {
    struct last_node_s *root;
};

struct last_node_s {
    list_t *block;
    int num_children;
    struct last_node_s **children;
};

int parser_parse(fluid_t *ctx)
{
    ARG_UNUSED(ctx);
    return 0;
}

void parser_teardown(fluid_t *ctx)
{
    ARG_UNUSED(ctx);
}
