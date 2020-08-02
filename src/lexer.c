/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fluid.h"
#include "lexer.h"

#define TOK_BUF_MAXLEN       256

enum lexer_state_e {
	LEXER_BLOCK_STATE_DATA,
	LEXER_BLOCK_STATE_OBJECT,
	LEXER_BLOCK_STATE_TAG,
	LEXER_BLOCK_STATE_ERROR,
};

void lexer_free_block(lexer_block_t *blk)
{
	if (blk->type == LEXER_BLOCK_TAG) {
		safe_free(blk->tok.tag.keyword);
		safe_free(blk->tok.tag.tokens);
	}
	else if (blk->type == LEXER_BLOCK_OBJECT) {
		safe_free(blk->tok.obj.identifier);
		safe_free(blk->tok.obj.filters);
	}
	else if (blk->type == LEXER_BLOCK_DATA) {
		string_destroy(&blk->content);
	}
	free(blk);
}

void lexer_free_blocks(fluid_t *ctx)
{
	lexer_block_t *blk;
	node_t *p;

	p = ctx->lex_blocks.head;
	while (p) {
		blk = CONTAINER_OF(p, lexer_block_t, node);
		p = p->next;
		lexer_free_block(blk);
	}
}

int lexer_add_block(fluid_t *ctx, enum lexer_block_e type,
		    size_t pos, size_t len)
{
	lexer_block_t *blk;

	if (len <= 0 || type <= LEXER_BLOCK_NONE ||
	    type >= LEXER_BLOCK_SENTINEL) {
		return -1;
	}

	blk = safe_zalloc(sizeof(lexer_block_t));
	if (blk == NULL) {
		printf("Lexer alloc failed\n");
		exit(-1);
	}
	blk->type = type;
	string_create(&blk->content, ctx->buf + pos, len);
	list_append(&ctx->lex_blocks, &blk->node);
	return 0;
}

int lexer_lex_blocks(fluid_t *ctx)
{
	char c1, c2;
	size_t current = 0, start = 0;
	enum lexer_state_e state = LEXER_BLOCK_STATE_DATA;

	while (current < (ctx->buf_size - 1)) {
		c1 = ctx->buf[current];
		c2 = ctx->buf[current + 1];
		switch(state) {
		case LEXER_BLOCK_STATE_DATA:
			if (c1 == '{' && c2 == '%') {
				lexer_add_block(ctx, LEXER_BLOCK_DATA, start,
						current - start);
				start = current;
				state = LEXER_BLOCK_STATE_TAG;
				break;
			}
			if (c1 == '{' && c2 == '{') {
				lexer_add_block(ctx, LEXER_BLOCK_DATA, start,
						current - start);
				start = current;
				state = LEXER_BLOCK_STATE_OBJECT;
				break;
			}
			break;
		case LEXER_BLOCK_STATE_TAG:
			if (c1 == '%' && c2 == '}') {
				/* +2 to include `%}` in current block */
				lexer_add_block(ctx, LEXER_BLOCK_TAG, start,
						current - start + 2);
				start = current + 2;
				state = LEXER_BLOCK_STATE_DATA;
			}
			break;
		case LEXER_BLOCK_STATE_OBJECT:
			if (c1 == '}' && c2 == '}') {
				/* +2 to include `}}` in current block */
				lexer_add_block(ctx, LEXER_BLOCK_OBJECT, start,
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
	lexer_add_block(ctx, LEXER_BLOCK_DATA, start, current - start);
	return 0;
}

void tag_clean(string_t *s)
{
	if (s->buf[0] == '{' && s->buf[1] == '%' &&
	    s->buf[s->len - 2] == '%' && s->buf[s->len - 1] == '}') {
		s->buf[0] = ' ';
		s->buf[1] = ' ';
		s->buf[s->len - 2] = ' ';
		s->buf[s->len - 1] = ' ';
	}
}

void obj_clean(string_t *s)
{
	if (s->buf[0] == '{' && s->buf[1] == '{' &&
	    s->buf[s->len - 2] == '}' && s->buf[s->len - 1] == '}') {
		s->buf[0] = ' ';
		s->buf[1] = ' ';
		s->buf[s->len - 2] = ' ';
		s->buf[s->len - 1] = ' ';
	}
}

int lexer_tokenize_tag(lexer_block_t *blk)
{
	char *tok, *rest;

	tag_clean(&blk->content);
	tok = strtok_r(blk->content.buf, " ", &rest);
	if (tok == NULL) {
		printf("lexer: failed to extract keyword\n");
		return -1;
	}
	blk->tok.tag.keyword = strdup(tok);
	split_string(rest, " ", &blk->tok.tag.tokens);
	return 0;
}

int lexer_tokenize_object(lexer_block_t *blk)
{
	char *filter, *tok, *rest;

	obj_clean(&blk->content);
	tok = strtok_r(blk->content.buf, " ", &rest);
	if (tok == NULL) {
		printf("lexer: failed to extract identifier\n");
		return -1;
	}
	blk->tok.obj.identifier = strdup(tok);
	blk->tok.obj.filters = NULL;
	filter = strchr(rest, '|');
	if (filter != NULL) {
		split_string(rest + 1, "| ", &blk->tok.obj.filters);
	}
	return 0;
}

int lexer_lex_tokens(fluid_t *ctx)
{
	lexer_block_t *blk;
	node_t *p = ctx->lex_blocks.head;

	while (p) {
		blk = CONTAINER_OF(p, lexer_block_t, node);
		if (blk->type == LEXER_BLOCK_TAG) {
			if (lexer_tokenize_tag(blk) != 0) {
				printf("lexer: tokenize tag failed\n");
				return -1;
			}
			string_destroy(&blk->content);
		}
		if (blk->type == LEXER_BLOCK_OBJECT) {
			if (lexer_tokenize_object(blk) != 0) {
				printf("lexer: tokenize object failed\n");
				return -1;
			}
			string_destroy(&blk->content);
		}
		p = p->next;
	}
	return 0;
}

void lexer_setup(fluid_t *ctx)
{
	list_init(&ctx->lex_blocks);
}

int lexer_lex(fluid_t *ctx)
{
	/* State1: Identify data/tag/object blocks */
	if (lexer_lex_blocks(ctx) != 0) {
		printf("lexer: failed to identify blocks\n");
		return -1;
	}

	/* State2: Lex tag and object blocks for more information */
	if (lexer_lex_tokens(ctx) != 0) {
		printf("lexer: failed to identify blocks\n");
		return -1;
	}

	return 0;
}

void lexer_teardown(fluid_t *ctx)
{
	lexer_free_blocks(ctx);
}
