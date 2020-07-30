/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <utils/utils.h>
#include <utils/list.h>

enum lexer_token_e {
	LEXER_TOKEN_NONE,
	LEXER_TOKEN_DATA,
	LEXER_TOKEN_OBJECT,
	LEXER_TOKEN_TAG,
	LEXER_TOKEN_SENTINEL,
};

typedef struct {
	node_t node;
	enum lexer_token_e type;
	size_t position;
	size_t length;
} lexer_token_t;

typedef struct {
	list_t tokens;
} lexer_t;

void lexer_init(lexer_t *ctx);
int lexer_lex(lexer_t *ctx, char *buf, size_t size);
void lexer_free(lexer_t *ctx);

#endif /* _COMMON_H_ */
