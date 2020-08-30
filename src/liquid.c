/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "liquid.h"

#define LIQ_KW_MAXLEN                  32
#define LIQ_BLK_SUB_KW_COUNT           3

/* Keyword attribute flags */
#define LIQ_KW_F_NONE                  0x00000000 /* place holder */
#define LIQ_KW_F_BARE                  0x00000001 /* can appear anywhere */
#define LIQ_KW_F_ENDTAG                0x00000002 /* this is an end* keyword */
#define LIQ_KW_F_ENCLOSED              0x00000004 /* must appear only inside a block */
#define LIQ_KW_F_LONE                  0x00000008 /* this is a keword-only tag */
#define LIQ_KW_F_ENCLOSING             0x00000016 /* this is a keword-only tag */
#define LIQ_KW_F_IS_END                (LIQ_KW_F_ENDTAG | LIQ_KW_F_LONE)

#define KW_HAS_ATTR(kw, attr)          ((liq_kw[kw].attrib & (attr)) == (attr))

typedef struct {
    const char *literal;
    uint32_t attrib;
} liq_kw_t;

#define LIQ_OP_TO_CSTR(x)           [x] = #x,

const char * const liq_kw_to_cstr[] = {
    LIQ_KW_MACRO(LIQ_OP_TO_CSTR)
};

#undef LIQ_OP_TO_CSTR

static liq_kw_t liq_kw[LIQ_KW_SENTINEL] = {
    [LIQ_KW_NONE]            = { "NONE",        LIQ_KW_F_NONE },
    [LIQ_KW_ASSIGN]          = { "assign",      LIQ_KW_F_BARE },
    [LIQ_KW_BREAK]           = { "break",       LIQ_KW_F_LONE | LIQ_KW_F_ENCLOSED },
    [LIQ_KW_CAPTURE]         = { "capture",     LIQ_KW_F_NONE | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_CASE]            = { "case",        LIQ_KW_F_NONE | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_WHEN]            = { "when",        LIQ_KW_F_ENCLOSED | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_COMMENT]         = { "comment",     LIQ_KW_F_LONE | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_CONTINUE]        = { "continue",    LIQ_KW_F_LONE | LIQ_KW_F_ENCLOSED },
    [LIQ_KW_DECREMENT]       = { "decrement",   LIQ_KW_F_BARE },
    [LIQ_KW_FOR]             = { "for",         LIQ_KW_F_NONE | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_IF]              = { "if",          LIQ_KW_F_NONE | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_ELSIF]           = { "elsif",       LIQ_KW_F_ENCLOSED | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_ELSE]            = { "else",        LIQ_KW_F_ENCLOSED | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_INCREMENT]       = { "increment",   LIQ_KW_F_BARE },
    [LIQ_KW_INCLUDE]         = { "include",     LIQ_KW_F_BARE },
    [LIQ_KW_RAW]             = { "raw",         LIQ_KW_F_LONE | LIQ_KW_F_ENCLOSING },
    [LIQ_KW_UNLESS]          = { "unless",      LIQ_KW_F_NONE | LIQ_KW_F_ENCLOSING },
};

typedef struct {
    enum liq_kw being;
    enum liq_kw end;
    enum liq_kw sub_kw[LIQ_BLK_SUB_KW_COUNT]; /* increase as needed */
} liq_blk_t;

static liq_blk_t liq_blk[LIQ_BLK_SENTINEL] = {
    [LIQ_BLK_CASE]     = { LIQ_KW_CASE,    LIQ_KW_ENDCASE,      { LIQ_KW_WHEN,  LIQ_KW_ELSE } },
    [LIQ_BLK_CAPTURE]  = { LIQ_KW_CAPTURE, LIQ_KW_ENDCAPTURE, {} },
    [LIQ_BLK_COMMENT]  = { LIQ_KW_COMMENT, LIQ_KW_ENDCOMMENT, {} },
    [LIQ_BLK_FOR]      = { LIQ_KW_FOR,     LIQ_KW_ENDFOR,     { LIQ_KW_ELSE,  LIQ_KW_BREAK, LIQ_KW_CONTINUE }  },
    [LIQ_BLK_IF]       = { LIQ_KW_IF,      LIQ_KW_ENDIF,      { LIQ_KW_ELSIF, LIQ_KW_ELSE } },
    [LIQ_BLK_RAW]      = { LIQ_KW_RAW,     LIQ_KW_ENDRAW,     {} },
    [LIQ_BLK_UNLESS]   = { LIQ_KW_UNLESS,  LIQ_KW_ENDUNLESS,  {} },
};

enum liq_kw liquid_get_kw(const char *literal)
{
    enum liq_kw i;
    enum liq_blk j;
    int len, is_end = 0;

    if (strncmp(literal, "end", 3) == 0) {
        literal += 3;
        is_end = true;
    }

    len = strlen(literal);
    if (len == 0 || len >= LIQ_KW_MAXLEN)
        return LIQ_KW_NONE;

    for (i = 1; i < LIQ_KW_SENTINEL; i++) {
        if (strncmp(literal, liq_kw[i].literal, len) == 0)
            break;
    }
    if (i >= LIQ_KW_SENTINEL)
        return LIQ_KW_NONE;
    if (!is_end)
        return i;
    for (j = 0; j < LIQ_BLK_SENTINEL; j++) {
        if (liq_blk[j].being == i)
            return liq_blk[j].end;
    }
    return LIQ_KW_NONE;
}

enum liq_operators liquid_get_optor(const char *op)
{
    switch (op[0]) {
    case '<':
    {
        if (op[1] == 0)
            return LIQ_OP_LESS;
        if (op[1] == '=')
            return LIQ_OP_LESS_EQUAL;
        break;
    }
    case '>':
    {
        if (op[1] == 0)
            return LIQ_OP_GREAT;
        if (op[1] == '=')
            return LIQ_OP_GREAT_EQUAL;
        break;
    }
    case '!':
    {
        if (op[1] == '=')
            return LIQ_OP_NOT_EQUAL;
        break;
    }
    case '=':
    {
        if (op[1] == '=')
            return LIQ_OP_EQUAlS;
        break;
    }
    case '&':
    {
        if (op[1] == '&')
            return LIQ_OP_LOGIC_AND;
        break;
    }
    case '|':
    {
        if (op[1] == '|')
            return LIQ_OP_LOGIC_OR;
        break;
    }
    case 'c':
    {
        if (!strcmp(op, "contains"))
            return LIQ_OP_CONTAINS;
    }
    default:
    {
        break;
    }
    }
    return LIQ_OP_SENTINEL;
}

enum liq_kw liquid_get_start_tag(enum liq_kw kw)
{
    for (int i = 0; i < LIQ_BLK_SENTINEL; i++) {
        if (liq_blk[i].end == kw)
            return liq_blk[i].being;
    }
    return LIQ_KW_NONE;
}

enum liq_kw liquid_get_end_tag(enum liq_kw kw)
{
    for (int i = 0; i < LIQ_BLK_SENTINEL; i++) {
        if (liq_blk[i].being == kw)
            return liq_blk[i].end;
    }
    return LIQ_KW_NONE;
}

bool liquid_is_block_begin(enum liq_kw kw)
{
    enum liq_blk i;

    if (kw >= LIQ_KW_SENTINEL)
        return false;

    for (i = 0; i < LIQ_BLK_SENTINEL; i++) {
        if (liq_blk[i].being == kw)
            return true;
    }
    return false;
}

bool liquid_is_block_end(enum liq_kw kw)
{
    enum liq_blk i;

    if (kw <= LIQ_KW_SENTINEL)
        return false;

    for (i = 0; i < LIQ_BLK_SENTINEL; i++) {
        if (liq_blk[i].end == kw)
            return true;
    }
    return false;
}

bool liquid_is_tag_bare(enum liq_kw kw)
{
    return KW_HAS_ATTR(kw, LIQ_KW_F_BARE);
}

bool liquid_is_tag_lone(enum liq_kw kw)
{
    return KW_HAS_ATTR(kw, LIQ_KW_F_LONE);
}

bool liquid_is_tag_enclosed(enum liq_kw kw)
{
    return KW_HAS_ATTR(kw, LIQ_KW_F_ENCLOSED);
}

bool liquid_is_tag_enclosing(enum liq_kw kw)
{
    return KW_HAS_ATTR(kw, LIQ_KW_F_ENCLOSING);
}

enum liq_blk liquid_get_blk(enum liq_kw kw)
{
    enum liq_blk i;

    for (i = 0; i < LIQ_BLK_SENTINEL; i++) {
        if (kw > LIQ_KW_SENTINEL && liq_blk[i].end == kw) {
            return i;
        }
        else if (liq_blk[i].being == kw) {
            return i;
        }
    }

    return LIQ_BLK_NONE;
}

bool liquid_is_valid(enum liq_blk parent, enum liq_kw kw)
{
    int i;

    if (parent == LIQ_BLK_NONE) {
        /* only a new tag_open or a bare tag can appear at level 1 */
        return liquid_is_block_begin(kw) || KW_HAS_ATTR(kw, LIQ_KW_F_BARE);
    }

    if (!KW_HAS_ATTR(kw, LIQ_KW_F_ENCLOSED)) {
        /* Any block can be inside any other block */
        return true;
    }

    for (i = 0; i < LIQ_BLK_SUB_KW_COUNT; i++) {
        if (liq_blk[parent].sub_kw[i] == kw) {
            return true;
        }
    }

    return false;
}


const char *liquid_get_cstr(enum liq_kw kw)
{
    return liq_kw_to_cstr[kw];
}