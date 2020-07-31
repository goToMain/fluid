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
	size_t position;
	size_t length;
	lexer_token_t tok;
} lexer_block_t;

typedef struct {
	char *buf;
	size_t buf_size;
	list_t blocks; /* list of nodes of type lexer_block_t */
} lexer_t;

void lexer_init(lexer_t *ctx, char *buf, size_t size);
int lexer_lex(lexer_t *ctx);
void lexer_free(lexer_t *ctx);

#endif /* _LEXER_H_ */
