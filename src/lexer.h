/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <utils/utils.h>
#include <utils/list.h>

enum lexer_block_e {
	LEXER_BLOCK_NONE,
	LEXER_BLOCK_DATA,
	LEXER_BLOCK_OBJECT,
	LEXER_BLOCK_TAG,
	LEXER_BLOCK_SENTINEL,
};

typedef struct {
	char *keyword;
	char **tokens;
} lexer_token_tag_t;

typedef struct {
	char *identifier;
	char **filters;
} lexer_token_obj_t;

typedef union {
	lexer_token_tag_t tag;
	lexer_token_obj_t obj;
} lexer_token_t;

typedef struct {
	node_t node;
	enum lexer_block_e type;
	string_t content;
	lexer_token_t tok;
} lexer_block_t;

void lexer_setup(fluid_t *ctx);
int  lexer_lex(fluid_t *ctx);
void lexer_teardown(fluid_t *ctx);
void lexer_free_block(lexer_block_t *blk);

#endif /* _LEXER_H_ */
