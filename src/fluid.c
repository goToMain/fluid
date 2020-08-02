/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <utils/file.h>

#include "fluid.h"
#include "lexer.h"
#include "parser.h"
#include "liquid.h"

fluid_t *fluid_load(const char *dirname, const char *filename)
{
	char *path = NULL;
	FILE *fd = NULL;
	fluid_t *ctx = NULL;

	path = path_join(dirname, filename);
	if (path == NULL) {
		printf("fluid: join '%s' and '%s' failed\n", dirname, filename);
		return NULL;
	}

	fd = fopen(path, "r");
	if (fd == NULL) {
		printf("fluid: failed to open '%s'\n", path);
		free(path);
		return NULL;
	}

	ctx = calloc(1, sizeof(fluid_t));
	if (ctx == NULL) {
		printf("fluid: failed alloc ctx\n");
		goto error;
	}

	if (file_read_all(fd, &ctx->buf, &ctx->buf_size) != 0) {
		printf("fluid: failed to read file %s\n", filename);
		goto error;
	}

	if (path_extract(path, &ctx->dirname, &ctx->filename)) {
		printf("Failed to extract dirname/basename from '%s'", path);
		goto error;
	}

	free(path);
	fclose(fd);
	return ctx;

error:
	if (ctx) {
		safe_free(ctx->filename);
		safe_free(ctx->dirname);
		free(ctx);
	}
	free(path);
	fclose(fd);
	return NULL;
}

void fluid_destroy_context(fluid_t *ctx)
{
	free(ctx->filename);
	free(ctx->dirname);
	free(ctx);
}

void fluid_render_data(fluid_t *ctx)
{
	lexer_block_t *blk;
	node_t *p;

	p = ctx->lex_blocks.head;
	printf("\n---\n\x1B[33m");
	while (p) {
		blk = CONTAINER_OF(p, lexer_block_t, node);
		p = p->next;
		if (blk->type == LEXER_BLOCK_DATA) {
			printf("%s", blk->content.buf);
		}
	}
	printf("\x1B[0m\n---\n");
}

void fluid_explode_sub_ctx(fluid_t *ctx, lexer_block_t *blk, fluid_t *sub_ctx)
{
	list_insert_nodes(&ctx->lex_blocks, &blk->node, &sub_ctx->lex_blocks);
	list_remove_node(&ctx->lex_blocks, &blk->node);
	lexer_free_block(blk);

	/* empty sub_ctx blocks to prevent lexer_teardown() from free-ing them */
	sub_ctx->lex_blocks.head = NULL;
	sub_ctx->lex_blocks.tail = NULL;
}

int fluid_handle_includes(fluid_t *ctx)
{
	fluid_t *sub_ctx = NULL;
	lexer_block_t *blk;
	node_t *p;

	p = ctx->lex_blocks.head;
	while (p) {
		blk = CONTAINER_OF(p, lexer_block_t, node);
		p = p->next;
		if (blk->type == LEXER_BLOCK_TAG &&
		    liquid_get_kw(blk->tok.tag.keyword) == LIQ_KW_INCLUDE &&
		    blk->tok.tag.tokens && blk->tok.tag.tokens[0]) {
			sub_ctx = fluid_load(ctx->dirname, blk->tok.tag.tokens[0]);
			if (sub_ctx == NULL) {
				return -1;
			}
			lexer_setup(sub_ctx);
			if (lexer_lex(sub_ctx) != 0) {
				return -1;
			}
			if (fluid_handle_includes(sub_ctx)) {
				return -1;
			}
			fluid_explode_sub_ctx(ctx, blk, sub_ctx);
			lexer_teardown(sub_ctx);
			fluid_destroy_context(sub_ctx);
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	fluid_t *ctx;

	if (argc != 2) {
		printf("Error: usage: %s: filename.html", argv[0]);
		return -1;
	}

	ctx = fluid_load(NULL, argv[1]);
	if (ctx == NULL) {
		return -1;
	}

	lexer_setup(ctx);
	if (lexer_lex(ctx) != 0) {
		return -1;
	}

	if (fluid_handle_includes(ctx)) {
		return -1;
	}

	parser_setup(ctx);
	if (parser_parse(ctx) != 0) {
		return -1;
	}

	fluid_render_data(ctx);

	lexer_teardown(ctx);
	parseer_teardown(ctx);
	fluid_destroy_context(ctx);
	return 0;
}
