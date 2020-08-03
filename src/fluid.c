/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <utils/file.h>

#include "fluid.h"
#include "lexer.h"
#include "parser.h"
#include "liquid.h"

fluid_t *fluid_load(const char *dirname, const char *filename)
{
    char *path = NULL;
    FILE *fd = NULL;
    fluid_t *ctx = NULL;

    path = path_join(dirname, filename);
    if (path == NULL) {
        printf("fluid: join '%s' and '%s' failed\n", dirname, filename);
        return NULL;
    }

    fd = fopen(path, "r");
    if (fd == NULL) {
        printf("fluid: failed to open '%s'\n", path);
        safe_free(path);
        return NULL;
    }

    ctx = safe_calloc(1, sizeof(fluid_t));
    if (file_read_all(fd, &ctx->buf, &ctx->buf_size) != 0) {
        printf("fluid: failed to read file %s\n", filename);
        goto error;
    }

    if (path_extract(path, &ctx->dirname, &ctx->filename)) {
        printf("Failed to extract dirname/basename from '%s'", path);
        goto error;
    }

    safe_free(path);
    fclose(fd);
    return ctx;

error:
    if (ctx) {
        safe_free(ctx->filename);
        safe_free(ctx->dirname);
        safe_free(ctx);
    }
    safe_free(path);
    fclose(fd);
    return NULL;
}

void fluid_destroy_context(fluid_t *ctx)
{
    safe_free(ctx->filename);
    safe_free(ctx->dirname);
    safe_free(ctx);
}

void fluid_render_data(fluid_t *ctx)
{
    lexer_block_t *blk;
    node_t *p;

    p = ctx->lex_blocks.head;
    printf("\n---\n\x1B[33m");
    while (p) {
        blk = CONTAINER_OF(p, lexer_block_t, node);
        p = p->next;
        if (blk->type == LEXER_BLOCK_DATA) {
            printf("%s", blk->content.buf);
        }
    }
    printf("\x1B[0m\n---\n");
}

void fluid_explode_sub_ctx(fluid_t *ctx, lexer_block_t *blk, fluid_t *sub_ctx)
{
    list_insert_nodes(&ctx->lex_blocks, &blk->node,
              sub_ctx->lex_blocks.head, sub_ctx->lex_blocks.tail);

    lexer_remove_block(ctx, blk);

    /* empty sub_ctx blocks to prevent lexer_teardown() from free-ing them */
    sub_ctx->lex_blocks.head = NULL;
    sub_ctx->lex_blocks.tail = NULL;
}

int fluid_remove_blocks(fluid_t *ctx, lexer_block_t *start, lexer_block_t *end)
{
    if (list_remove_nodes(&ctx->lex_blocks, &start->node, &end->node)) {
        printf("fluid: block remove failed\n");
        return -1;
    }

    while (start != end) {
        lexer_remove_block(ctx, start);
        start = CONTAINER_OF(start->node.next, lexer_block_t, node);
    }
    lexer_remove_block(ctx, end);
    return 0;
}

void fluid_handle_raw_blocks(fluid_t *ctx, lexer_block_t *start, lexer_block_t *end)
{
    lexer_block_t *p;

    p = CONTAINER_OF(start->node.next, lexer_block_t, node);
    lexer_remove_block(ctx, start);
    while (p != end) {
        lexer_block_cast_to_data(p);
        p = CONTAINER_OF(p->node.next, lexer_block_t, node);
    }
    lexer_remove_block(ctx, end);
}

int fluid_preprocessor(fluid_t *ctx)
{
    node_t *p;
    lexer_block_t *blk;
    fluid_t *sub_ctx;
    lexer_block_t *comment_start = NULL;
    lexer_block_t *raw_start = NULL;

    p = ctx->lex_blocks.head;
    while (p) {
        blk = CONTAINER_OF(p, lexer_block_t, node);
        p = p->next;

        /* strip comments */
        if (comment_start) {
            if (blk->type == LEXER_BLOCK_TAG &&
                blk->tok.tag.keyword == LIQ_KW_ENDCOMMENT)
            {
                if (fluid_remove_blocks(ctx, comment_start, blk))
                    return -1;
                comment_start = NULL;
            }
            continue;
        }
        else if (blk->type == LEXER_BLOCK_TAG &&
                 blk->tok.tag.keyword == LIQ_KW_COMMENT)
        {
            comment_start = blk;
            continue;
        }

        /* force mark raw blocks as data blocks */
        if (raw_start) {
            if (blk->type == LEXER_BLOCK_TAG &&
                blk->tok.tag.keyword == LIQ_KW_ENDRAW)
            {
                fluid_handle_raw_blocks(ctx, raw_start, blk);
                raw_start = NULL;
            }
            continue;
        }
        else if (blk->type == LEXER_BLOCK_TAG &&
                 blk->tok.tag.keyword == LIQ_KW_RAW)
        {
            raw_start = blk;
            continue;
        }

        /* include files */
        if (blk->type == LEXER_BLOCK_TAG &&
            blk->tok.tag.keyword == LIQ_KW_INCLUDE &&
            blk->tok.tag.tokens && blk->tok.tag.tokens[0])
        {
            sub_ctx = fluid_load(ctx->dirname, blk->tok.tag.tokens[0]);
            if (sub_ctx == NULL) {
                return -1;
            }
            lexer_setup(sub_ctx);
            if (lexer_lex(sub_ctx) != 0) {
                return -1;
            }
            if (fluid_preprocessor(sub_ctx)) {
                return -1;
            }
            fluid_explode_sub_ctx(ctx, blk, sub_ctx);
            lexer_teardown(sub_ctx);
            fluid_destroy_context(sub_ctx);
        }
    }

    /**
     * After preprocessing there may be consecutive data blocks
     * so call lexer_grabage_collect() to squash them.
     */
    lexer_grabage_collect(ctx);

    return 0;
}

int main(int argc, char *argv[])
{
    fluid_t *ctx;

    if (argc != 2) {
        printf("Error: usage: %s: filename.html", argv[0]);
        return -1;
    }

    ctx = fluid_load(NULL, argv[1]);
    if (ctx == NULL) {
        return -1;
    }

    lexer_setup(ctx);
    if (lexer_lex(ctx) != 0) {
        return -1;
    }

    if (fluid_preprocessor(ctx)) {
        return -1;
    }

    parser_setup(ctx);
    if (parser_parse(ctx) != 0) {
        return -1;
    }

    fluid_render_data(ctx);

    lexer_teardown(ctx);
    parser_teardown(ctx);
    fluid_destroy_context(ctx);

    return 0;
}
