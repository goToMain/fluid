/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLUID_H_
#define _FLUID_H_

#include <stddef.h>
#include <utils/list.h>
#include <utils/utils.h>
#include <utils/strutils.h>
#include <utils/strlib.h>

typedef struct fluid_s {
    char *filename;
    char *dirname;
    char *buf;
    size_t buf_size;
    list_t lex_blocks; /* list of nodes of type lexer_block_t */
} fluid_t;

#endif /* _FLUID_H_ */
