/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

enum lexer_state_e {
	LEXER_STATE_DATA,
	LEXER_STATE_OBJECT,
	LEXER_STATE_TAG,
	LEXER_STATE_ERROR,
};

lexer_token_t *lexer_new_token(enum lexer_token_e type, size_t pos, size_t len)
{
	lexer_token_t *t;

	t = calloc(1, sizeof(lexer_token_t));
	if (t == NULL) {
		printf("Lexer alloc failed\n");
		exit(-1);
	}
	t->type = type;
	t->position = pos;
	t->length = len;
	return t;
}

void lexer_init(lexer_t *ctx)
{
	list_init(&ctx->tokens);
}

void lexer_free(lexer_t *ctx)
{
	lexer_token_t *t;
	node_t *p;

	p = ctx->tokens.head;
	while (p) {
		t = CONTAINER_OF(p, lexer_token_t, node);
		free(t);
		p = p->next;
	}
}

void lexer_token_append(lexer_t *ctx, lexer_token_t *t)
{
	list_append(&ctx->tokens, &t->node);
}

int lexer_lex(lexer_t *ctx, char *buf, size_t size)
{
	char c1, c2;
	size_t current = 0, start = 0;
	lexer_token_t *t = NULL;
	enum lexer_state_e state = LEXER_STATE_DATA;

	while (current < (size - 1)) {
		c1 = buf[current];
		c2 = buf[current + 1];
		switch(state) {
		case LEXER_STATE_DATA:
			if (c1 == '{' && c2 == '%') {
				t = lexer_new_token(LEXER_TOKEN_DATA, start,
						    current - start - 1);
				lexer_token_append(ctx, t);
				start = current;
				state = LEXER_STATE_TAG;
				break;
			}
			if (c1 == '{' && c2 == '{') {
				t = lexer_new_token(LEXER_TOKEN_DATA, start,
						    current - start - 1);
				lexer_token_append(ctx, t);
				start = current;
				state = LEXER_STATE_OBJECT;
				break;
			}
			break;
		case LEXER_STATE_TAG:
			if (c1 == '%' && c2 == '}') {
				t = lexer_new_token(LEXER_TOKEN_TAG, start,
						    current - start + 2);
				lexer_token_append(ctx, t);
				start = current + 2;
				state = LEXER_STATE_DATA;
			}
			break;
		case LEXER_STATE_OBJECT:
			if (c1 == '}' && c2 == '}') {
				t = lexer_new_token(LEXER_TOKEN_OBJECT, start,
						    current - start + 2);
				lexer_token_append(ctx, t);
				start = current + 2;
				state = LEXER_STATE_DATA;
			}
			break;
		case LEXER_STATE_ERROR:
			break;
		}
		current += 1;
	}
	if (current > start) {
		t = lexer_new_token(LEXER_TOKEN_DATA, start, current - start);
		t->type = LEXER_TOKEN_DATA;
		t->position = start;
		t->length = current - start;
		lexer_token_append(ctx, t);
	}
	return 0;
}
