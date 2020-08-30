/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utils/logger.h>

#include "fluid.h"
#include "lexer.h"

LOGGER_MODULE_EXTERN(fluid, lexer);

enum lexer_state {
    LEXER_BLOCK_STATE_DATA,
    LEXER_BLOCK_STATE_OBJECT,
    LEXER_BLOCK_STATE_TAG,
    LEXER_BLOCK_STATE_ERROR,
};

/* Transform "{% TAG %}" or "{{ OBJ }}" as "TAG" or "OBJ" */
static int lexer_clean_liquid_markup(char *buf, size_t len)
{
    buf[0] = ' ';
    buf[1] = ' ';
    buf[len - 2] = ' ';
    buf[len - 1] = ' ';
    return strip(buf);
}

static void lexer_block_free(lexer_block_t *blk)
{
    if (blk->type == LEXER_BLOCK_TAG) {
        safe_free(blk->tok.tag.tokens);
    }
    else if (blk->type == LEXER_BLOCK_OBJECT) {
        safe_free(blk->tok.obj.identifier);
        safe_free(blk->tok.obj.filters);
    }
    string_destroy(&blk->content);
    safe_free(blk);
}

void lexer_remove_block(fluid_t *ctx, lexer_block_t *blk)
{
    list_remove_node(&ctx->lex_blocks, &blk->node);
    lexer_block_free(blk);
}

int lexer_merge_blocks(fluid_t *ctx, lexer_block_t *b1, lexer_block_t *b2)
{
    if (b1->type != b2->type)
        return -1;

    /* b1 and b2 are in order. So merge as-is and swap so b1 can be free-ed */
    if (string_merge(&b1->content, &b2->content)) {
        return -1;
    }
    memcpy(&b2->content, &b1->content, sizeof(string_t));
    b1->content.buf = NULL; // clear as it is now held in b2.content.buf
    lexer_remove_block(ctx, b1);
    return 0;
}

void lexer_block_cast_to_data(lexer_block_t *blk)
{
    if (blk->type == LEXER_BLOCK_TAG) {
        safe_free(blk->tok.tag.tokens);
    }
    else if (blk->type == LEXER_BLOCK_OBJECT) {
        safe_free(blk->tok.obj.identifier);
        safe_free(blk->tok.obj.filters);
    }
    blk->type = LEXER_BLOCK_DATA;
}

static int
lexer_block_add(fluid_t *ctx, enum lexer_block type, size_t pos, size_t len)
{
    lexer_block_t *blk;

    if (len <= 0 || type <= LEXER_BLOCK_NONE ||
        type >= LEXER_BLOCK_SENTINEL) {
        return -1;
    }

    blk = safe_calloc(1, sizeof(lexer_block_t));
    blk->type = type;
    string_create(&blk->content, ctx->buf + pos, len);
    list_append(&ctx->lex_blocks, &blk->node);
    return 0;
}

static void lexer_free_blocks(fluid_t *ctx)
{
    lexer_block_t *blk;
    node_t *p;

    p = ctx->lex_blocks.head;
    while (p) {
        blk = CONTAINER_OF(p, lexer_block_t, node);
        p = p->next;
        lexer_block_free(blk);
    }
}

int lexer_lex_blocks(fluid_t *ctx)
{
    char c1, c2;
    size_t current = 0, start = 0;
    enum lexer_state state = LEXER_BLOCK_STATE_DATA;

    while (current < (ctx->buf_size - 1)) {
        c1 = ctx->buf[current];
        c2 = ctx->buf[current + 1];
        switch(state) {
        case LEXER_BLOCK_STATE_DATA:
            if (c1 == '{' && c2 == '%') {
                lexer_block_add(ctx, LEXER_BLOCK_DATA, start,
                        current - start);
                start = current;
                state = LEXER_BLOCK_STATE_TAG;
                break;
            }
            if (c1 == '{' && c2 == '{') {
                lexer_block_add(ctx, LEXER_BLOCK_DATA, start,
                        current - start);
                start = current;
                state = LEXER_BLOCK_STATE_OBJECT;
                break;
            }
            break;
        case LEXER_BLOCK_STATE_TAG:
            if (c1 == '%' && c2 == '}') {
                /* +2 to include `%}` in current block */
                lexer_block_add(ctx, LEXER_BLOCK_TAG, start,
                        current - start + 2);
                start = current + 2;
                state = LEXER_BLOCK_STATE_DATA;
            }
            break;
        case LEXER_BLOCK_STATE_OBJECT:
            if (c1 == '}' && c2 == '}') {
                /* +2 to include `}}` in current block */
                lexer_block_add(ctx, LEXER_BLOCK_OBJECT, start,
                        current - start + 2);
                start = current + 2;
                state = LEXER_BLOCK_STATE_DATA;
            }
            break;
        case LEXER_BLOCK_STATE_ERROR:
            break;
        }
        current += 1;
    }
    if (state != LEXER_BLOCK_STATE_DATA) {
        /* at the end we must always be at a data block */
        return -1;
    }
    /* if there tailing chars, create a new block from them */
    lexer_block_add(ctx, LEXER_BLOCK_DATA, start, current - start);
    return 0;
}

int lexer_filter_parse(liq_filter_t *f, char *str)
{
    char *tok, args;
    int len, i;

    if((tok = strsep(&str, ":")) == NULL)
        return -1;

    lstrip_soft(tok); rstrip(tok);
    if ((f->id = get_filter_id(tok)) == LIQ_FILTER_NONE)
        return -1;

    args = liq_filter_arg_count(f->id);

    if (strisempty(str)) {
        if (args != 0)
            return -1;
        return 0;
    }

    for (i = 0; i < args; i++) {
        if((tok = strsep(&str, ",")) == NULL)
            return -1;

        lstrip_soft(tok); len = rstrip(tok);
        if (len > LIQ_FILTER_ARG_MAXLEN)
            return -1;

        strncpy(f->args[i], tok, LIQ_FILTER_ARG_MAXLEN);
    }

    if (!strisempty(str))
        return -1;

    return 0;
}

int lexer_tokenize_tag(lexer_block_t *blk)
{
    int len;
    char *tok, *p, *f, buf[256];

    if (blk->content.max_len >= 256) {
        LOG_ERR("object too large");
        return -1;
    }

    p = buf;
    strncpy(buf, blk->content.buf, blk->content.len);
    buf[blk->content.len] = '\0';
    if (lexer_clean_liquid_markup(buf, blk->content.len) == 0)
        return -1;

    if ((tok = strsep(&p, " ")) == NULL) {
        LOG_ERR("failed to extract keyword");
        return -1;
    }
    lstrip_soft(tok); rstrip(tok);
    blk->tok.tag.keyword = liquid_get_kw(tok);

    if (p && (f = strchr(p, '|')) != NULL) {
        f[0] = '\0';
        lstrip_soft(f); len = rstrip(f);
        if (!len || lexer_filter_parse(&blk->tok.tag.filter, f)) {
            LOG_ERR("tag filter syntax error");
            return -1;
        }
    }

    blk->tok.tag.tokens = NULL;
    if (p && split_string(p, " ", &blk->tok.tag.tokens)) {
        LOG_ERR("tag parse errors");
        return -1;
    }

    return 0;
}

int lexer_tokenize_object(lexer_block_t *blk)
{
    int i = 0;
    int num_filters;
    liq_filter_t *filters, *filter;
    char *tok, *p, buf[256];

    if (blk->content.max_len >= 256) {
        LOG_ERR("object too large");
        return -1;
    }

    p = buf;
    strncpy(buf, blk->content.buf, blk->content.len);
    buf[blk->content.len] = '\0';
    if (lexer_clean_liquid_markup(buf, blk->content.len) == 0)
        return -1;

    if ((tok = strsep(&p, " ")) == NULL) {
        LOG_ERR("failed to extract identifier");
        return -1;
    }
    blk->tok.obj.identifier = safe_strdup(tok);

    if (strisempty(p))
        return 0;

    if (*p != '|') {
        LOG_ERR("object found '%s' in place of filters", p);
        return -1;
    }
    p += 1; /* skip the leading '|' */
    num_filters = 1 + strcntchr(p, '|');
    filters = filter = safe_calloc(num_filters, sizeof(liq_filter_t));
    while (i < num_filters && (tok = strsep(&p, "|")) != NULL) {
        lstrip_soft(tok);
        if (strisempty(tok))
            break;
        if (lexer_filter_parse(filter, tok)) {
            LOG_ERR("object filter syntax error");
            break;
        }
        filter += 1;
        i += 1;
    }

    if (i != num_filters) {
        safe_free(filters);
        return -1;
    }

    blk->tok.obj.num_filters = num_filters;
    blk->tok.obj.filters = filters;
    return 0;
}

int lexer_lex_tokens(fluid_t *ctx)
{
    lexer_block_t *blk;

    LIST_FOREACH(&ctx->lex_blocks, p) {
        blk = CONTAINER_OF(p, lexer_block_t, node);
        if (blk->type == LEXER_BLOCK_TAG) {
            if (lexer_tokenize_tag(blk) != 0) {
                LOG_ERR("tokenize tag failed");
                return -1;
            }
        }
        if (blk->type == LEXER_BLOCK_OBJECT) {
            if (lexer_tokenize_object(blk) != 0) {
                LOG_ERR("tokenize object failed");
                return -1;
            }
        }
    }
    return 0;
}

void lexer_grabage_collect(fluid_t *ctx)
{
    lexer_block_t *cur, *prev = NULL;

    if (ctx->lex_blocks.head == NULL)
        return;

    LIST_FOREACH(&ctx->lex_blocks, p) {
        cur = CONTAINER_OF(p, lexer_block_t, node);
        if (prev && prev->type == LEXER_BLOCK_DATA &&
            cur->type == LEXER_BLOCK_DATA)
        {
            if (lexer_merge_blocks(ctx, prev, cur)) {
                LOG_INF("gc: failed to merge blocks. Skipped!");
            }
        }
        prev = cur;
    }
}

void lexer_setup(fluid_t *ctx)
{
    list_init(&ctx->lex_blocks);
}

int lexer_lex(fluid_t *ctx)
{
    /* State1: Identify data/tag/object blocks */
    if (lexer_lex_blocks(ctx) != 0)
        return -1;

    /* State2: Lex tag and object blocks for more information */
    if (lexer_lex_tokens(ctx) != 0)
        return -1;

    return 0;
}

void lexer_teardown(fluid_t *ctx)
{
    lexer_free_blocks(ctx);
}
