/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LIQUID_H_
#define _LIQUID_H_

#include <stdbool.h>
#include <stdint.h>

#define LIQ_KW_MACRO(OPERATION) \
        OPERATION(LIQ_KW_NONE)      \
        OPERATION(LIQ_KW_ASSIGN)    \
        OPERATION(LIQ_KW_BREAK)     \
        OPERATION(LIQ_KW_CAPTURE)   \
        OPERATION(LIQ_KW_CASE)      \
        OPERATION(LIQ_KW_WHEN)      \
        OPERATION(LIQ_KW_COMMENT)   \
        OPERATION(LIQ_KW_CONTINUE)  \
        OPERATION(LIQ_KW_DECREMENT) \
        OPERATION(LIQ_KW_FOR)       \
        OPERATION(LIQ_KW_IF)        \
        OPERATION(LIQ_KW_ELSIF)     \
        OPERATION(LIQ_KW_ELSE)      \
        OPERATION(LIQ_KW_INCREMENT) \
        OPERATION(LIQ_KW_INCLUDE)   \
        OPERATION(LIQ_KW_RAW)       \
        OPERATION(LIQ_KW_UNLESS)    \
        OPERATION(LIQ_KW_SENTINEL)  \
        OPERATION(LIQ_KW_ENDIF)     \
        OPERATION(LIQ_KW_ENDCAPTURE)\
        OPERATION(LIQ_KW_ENDCASE)   \
        OPERATION(LIQ_KW_ENDCOMMENT)\
        OPERATION(LIQ_KW_ENDFOR)    \
        OPERATION(LIQ_KW_ENDRAW)    \
        OPERATION(LIQ_KW_ENDUNLESS) \

#define LIQ_OP_TO_ENUM(x)           x,

enum liq_kw {
    LIQ_KW_MACRO(LIQ_OP_TO_ENUM)
};

#undef LIQ_OP_TO_ENUM

enum liq_blk {
    LIQ_BLK_CASE,
    LIQ_BLK_CAPTURE,
    LIQ_BLK_COMMENT,
    LIQ_BLK_FOR,
    LIQ_BLK_IF,
    LIQ_BLK_RAW,
    LIQ_BLK_UNLESS,
    LIQ_BLK_SENTINEL,
    LIQ_BLK_NONE,
};

enum liq_operators {
    LIQ_OP_LESS,
    LIQ_OP_GREAT,
    LIQ_OP_LESS_EQUAL,
    LIQ_OP_GREAT_EQUAL,
    LIQ_OP_EQUAlS,
    LIQ_OP_NOT_EQUAL,
    LIQ_OP_LOGIC_OR,
    LIQ_OP_LOGIC_AND,
    LIQ_OP_CONTAINS,
    LIQ_OP_SENTINEL
};

enum liq_kw liquid_get_kw(const char *literal);
enum liq_operators liquid_get_optor(const char *op);
enum liq_blk liquid_get_blk(enum liq_kw kw);
bool liquid_is_block_begin(enum liq_kw kw);
bool liquid_is_block_end(enum liq_kw kw);
bool liquid_is_tag_bare(enum liq_kw kw);
bool liquid_is_tag_lone(enum liq_kw kw);
bool liquid_is_tag_enclosing(enum liq_kw kw);
bool liquid_is_valid(enum liq_blk parent, enum liq_kw kw);

enum liq_kw liquid_get_start_tag(enum liq_kw kw);
enum liq_kw liquid_get_end_tag(enum liq_kw kw);
const char *liquid_get_cstr(enum liq_kw kw);

#endif /* _LIQUID_H_ */
