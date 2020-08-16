/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <getopt.h>
#include <unistd.h>
#include <utils/file.h>
#include <utils/logger.h>

#include "fluid.h"
#include "lexer.h"
#include "parser.h"
#include "liquid.h"

LOGGER_MODULE_DEFINE(fluid, LOG_ERR);

fluid_t *fluid_load(const char *dirname, const char *filename)
{
    char *path = NULL;
    FILE *fd = NULL;
    fluid_t *ctx = NULL;

    path = path_join(dirname, filename);
    if (path == NULL) {
        LOG_ERR("join '%s' and '%s' failed", dirname, filename);
        return NULL;
    }

    fd = fopen(path, "r");
    if (fd == NULL) {
        LOG_ERR("failed to open '%s'", path);
        safe_free(path);
        return NULL;
    }

    ctx = safe_calloc(1, sizeof(fluid_t));
    if (file_read_all(fd, &ctx->buf, &ctx->buf_size) != 0) {
        LOG_ERR("failed to read file %s", filename);
        goto error;
    }

    if (path_extract(path, &ctx->dirname, &ctx->filename)) {
        LOG_ERR("failed to extract dirname/basename from '%s'", path);
        goto error;
    }
    safe_free(path);
    fclose(fd);
    return ctx;

error:
    if (ctx) {
        safe_free(ctx->filename);
        safe_free(ctx->dirname);
        safe_free(ctx);
    }
    safe_free(path);
    fclose(fd);
    return NULL;
}

void fluid_destroy_context(fluid_t *ctx)
{
    safe_free(ctx->filename);
    safe_free(ctx->dirname);
    safe_free(ctx);
}

void fluid_render_data(fluid_t *ctx)
{
    lexer_block_t *blk;

    LIST_FOREACH(&ctx->lex_blocks, p) {
        blk = CONTAINER_OF(p, lexer_block_t, node);
        if (blk->type == LEXER_BLOCK_DATA) {
            fprintf(ctx->outfile, "%s", blk->content.buf);
        }
    }
}

void fluid_explode_sub_ctx(fluid_t *ctx, lexer_block_t *blk, fluid_t *sub_ctx)
{
    list_insert_nodes(&ctx->lex_blocks, &blk->node,
              sub_ctx->lex_blocks.head, sub_ctx->lex_blocks.tail);

    lexer_remove_block(ctx, blk);

    /* empty sub_ctx blocks to prevent lexer_teardown() from free-ing them */
    sub_ctx->lex_blocks.head = NULL;
    sub_ctx->lex_blocks.tail = NULL;
}

int fluid_remove_blocks(fluid_t *ctx, lexer_block_t *start, lexer_block_t *end)
{
    node_t *next;

    if (list_remove_nodes(&ctx->lex_blocks, &start->node, &end->node)) {
        LOG_ERR("block remove failed");
        return -1;
    }

    while (start != end) {
        next = start->node.next;
        lexer_remove_block(ctx, start);
        start = CONTAINER_OF(next, lexer_block_t, node);
    }
    lexer_remove_block(ctx, end);
    return 0;
}

void fluid_handle_raw_blocks(fluid_t *ctx, lexer_block_t *start, lexer_block_t *end)
{
    lexer_block_t *p;

    p = CONTAINER_OF(start->node.next, lexer_block_t, node);
    lexer_remove_block(ctx, start);
    while (p != end) {
        lexer_block_cast_to_data(p);
        p = CONTAINER_OF(p->node.next, lexer_block_t, node);
    }
    lexer_remove_block(ctx, end);
}

int fluid_preprocessor(fluid_t *ctx)
{
    node_t *p;
    lexer_block_t *blk;
    fluid_t *sub_ctx;
    lexer_block_t *comment_start = NULL;
    lexer_block_t *raw_start = NULL;

    p = ctx->lex_blocks.head;
    while (p) {
        blk = CONTAINER_OF(p, lexer_block_t, node);
        p = p->next;

        /* strip comments */
        if (comment_start) {
            if (blk->type == LEXER_BLOCK_TAG &&
                blk->tok.tag.keyword == LIQ_KW_ENDCOMMENT)
            {
                if (fluid_remove_blocks(ctx, comment_start, blk))
                    return -1;
                comment_start = NULL;
            }
            continue;
        }
        else if (blk->type == LEXER_BLOCK_TAG &&
                 blk->tok.tag.keyword == LIQ_KW_COMMENT)
        {
            comment_start = blk;
            continue;
        }

        /* force mark raw blocks as data blocks */
        if (raw_start) {
            if (blk->type == LEXER_BLOCK_TAG &&
                blk->tok.tag.keyword == LIQ_KW_ENDRAW)
            {
                fluid_handle_raw_blocks(ctx, raw_start, blk);
                raw_start = NULL;
            }
            continue;
        }
        else if (blk->type == LEXER_BLOCK_TAG &&
                 blk->tok.tag.keyword == LIQ_KW_RAW)
        {
            raw_start = blk;
            continue;
        }

        /* include files */
        if (blk->type == LEXER_BLOCK_TAG &&
            blk->tok.tag.keyword == LIQ_KW_INCLUDE &&
            blk->tok.tag.tokens && blk->tok.tag.tokens[0])
        {
            sub_ctx = fluid_load(ctx->dirname, blk->tok.tag.tokens[0]);
            if (sub_ctx == NULL) {
                return -1;
            }
            lexer_setup(sub_ctx);
            if (lexer_lex(sub_ctx) != 0) {
                return -1;
            }
            if (fluid_preprocessor(sub_ctx)) {
                return -1;
            }
            fluid_explode_sub_ctx(ctx, blk, sub_ctx);
            lexer_teardown(sub_ctx);
            fluid_destroy_context(sub_ctx);
        }
    }

    /**
     * After preprocessing there may be consecutive data blocks
     * so call lexer_grabage_collect() to squash them.
     */
    lexer_grabage_collect(ctx);

    return 0;
}

struct fluid_opts_s {
    char *infile;
    char *outfile;
    int verbosity;
} fluid_opts;

static const char *fluid_help[] = {
    "Usage: fluid [OPTIONS] <template_file> [-o <output_file>]",
    "",
    "OPTIONS:",
    "  outfile              Write output to file (defaults to stdout)",
    "  help                 Print this help text",
    "  version              Print fluid version",
    "  verbosity            Increase the verbosity (allows multiple)",
    NULL
};

void exit_help(int return_code)
{
    int i = 0;

    fprintf(stderr, "\n");
    while (fluid_help[i] != NULL) {
        fprintf(stderr, "%s\n", fluid_help[i]);
        i += 1;
    }
    fprintf(stderr, "\n");
    exit(return_code);
}

void exit_error(const char *msg)
{
    fprintf(stderr, "fluid: error: %s\n", msg);
    exit(-1);
}

void exit_version()
{
    fprintf(stderr, "fluid v%s\n", VERSION);
    exit(0);
}

void process_cli_opts(int argc, char *argv[])
{
    int c;
    int opt_ndx;
    static struct option opts[] = {
        { "help",       no_argument,       NULL,                   'h' },
        { "version",    no_argument,       NULL,                   'V' },
        { "outfile",    required_argument, NULL,                   'o' },
        { "verbose",    optional_argument, NULL,                   'v' },
        { NULL,         0,                 NULL,                    0  }
    };
    const char *opt_str =
        /* no_argument       */ "hV"
        /* required_argument */ "o:"
        /* optional_argument */ "v::"
    ;
    while ((c = getopt_long(argc, argv, opt_str, opts, &opt_ndx)) >= 0) {
        switch (c) {
        case 'o':
            if (fluid_opts.outfile)
                exit_error("Cannot pass multiple output files");
            fluid_opts.outfile = safe_strdup(optarg);
            break;
        case 'v':
            fluid_opts.verbosity += 1;
            if (optarg)
                fluid_opts.verbosity += strlen(optarg);
            break;
        case 'V':
            exit_version();
            break;
        case 'h':
            exit_help(0);
            break;
        case '?':
        default:
            exit_help(-1);
        }
    }
    argc -= optind;
    argv += optind;

    /* handle positional arguments */

    if (argc != 1)
        exit_error("no input files given. See --help");

    fluid_opts.infile = safe_strdup(argv[0]);
}

int main(int argc, char *argv[])
{
    fluid_t *ctx;

    process_cli_opts(argc, argv);

    ctx = fluid_load(NULL, fluid_opts.infile);
    if (ctx == NULL) {
        return -1;
    }

    ctx->outfile = stdout;
    if (fluid_opts.outfile) {
        ctx->outfile = fopen(fluid_opts.outfile, "w");
        if (ctx->outfile == NULL) {
            LOG_ERR("Failed to open out file %s", fluid_opts.outfile);
            return -1;
        }
    }

    lexer_setup(ctx);
    if (lexer_lex(ctx) != 0) {
        return -1;
    }

    if (fluid_preprocessor(ctx)) {
        return -1;
    }

    parser_setup(ctx);
    if (parser_parse(ctx) != 0) {
        return -1;
    }

    fluid_render_data(ctx);

    lexer_teardown(ctx);
    parser_teardown(ctx);
    fluid_destroy_context(ctx);

    return 0;
}
