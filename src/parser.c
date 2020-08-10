/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>

#include "parser.h"

pt_node_t *new_pt_node(pt_node_t *parent, enum pt_node_type type)
{
    pt_node_t *n;

    n = safe_calloc(1, sizeof(pt_node_t));
    n->type = type;
    n->parent = parent;
    return n;
}

int build_parse_tree(fluid_t *ctx)
{
    ARG_UNUSED(ctx);

    return 0;
}

void parser_setup(fluid_t *ctx)
{
    ARG_UNUSED(ctx);
}

int parser_parse(fluid_t *ctx)
{
    if (build_parse_tree(ctx))
        return -1;

    return 0;
}

void parser_teardown(fluid_t *ctx)
{
    ARG_UNUSED(ctx);
}
