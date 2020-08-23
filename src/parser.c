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

typedef struct {
    pt_node_t *root;
} parser_t;

int build_parse_tree(parser_t *ctx, list_t *lex_blocks)
{
    lexer_block_t *blk;

    ARG_UNUSED(ctx);

    LIST_FOREACH(lex_blocks, p) {
        blk = CONTAINER_OF(p, lexer_block_t, node);
        switch (blk->type) {
        case LEXER_BLOCK_DATA:

            break;
        case LEXER_BLOCK_OBJECT:
            break;
        case LEXER_BLOCK_TAG:
            break;
        default:
            break;
        }
    }
    return 0;
}

void parser_setup(fluid_t *ctx)
{
    parser_t *p;

    p = safe_malloc(sizeof(parser_t));
    p->root = NULL;

    ctx->parser_data = p;
}

int parser_parse(fluid_t *ctx)
{
    parser_t *p = ctx->parser_data;

    if (build_parse_tree(p, &ctx->lex_blocks))
        return -1;

    return 0;
}

void parser_teardown(fluid_t *ctx)
{
    parser_t *p = ctx->parser_data;

    safe_free(p);
}
