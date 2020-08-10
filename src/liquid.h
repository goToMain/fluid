/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LIQUID_H_
#define _LIQUID_H_

#include <stdbool.h>
#include <stdint.h>

enum liq_kw {
    LIQ_KW_NONE,
    LIQ_KW_ASSIGN,
    LIQ_KW_BREAK,
    LIQ_KW_CAPTURE,
    LIQ_KW_CASE,
    LIQ_KW_WHEN,
    LIQ_KW_COMMENT,
    LIQ_KW_CONTINUE,
    LIQ_KW_DECREMENT,
    LIQ_KW_FOR,
    LIQ_KW_IF,
    LIQ_KW_ELSIF,
    LIQ_KW_ELSE,
    LIQ_KW_INCREMENT,
    LIQ_KW_INCLUDE,
    LIQ_KW_RAW,
    LIQ_KW_UNLESS,
    LIQ_KW_SENTINEL,
    /* virtual keywords */
    LIQ_KW_ENDIF,
    LIQ_KW_ENDCAPTURE,
    LIQ_KW_ENDCASE,
    LIQ_KW_ENDCOMMENT,
    LIQ_KW_ENDFOR,
    LIQ_KW_ENDRAW,
    LIQ_KW_ENDUNLESS,
};

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
enum liq_blk liquid_get_blk(enum liq_kw kw);
bool liquid_is_block_begin(enum liq_kw kw);
bool liquid_is_block_end(enum liq_kw kw);
bool liquid_is_valid(enum liq_blk parent, enum liq_kw kw);

#endif /* _LIQUID_H_ */
