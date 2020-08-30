/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>

#include "parser.h"

/*
 * Tags: Three Types
 * 1. Tags with no end tag and self-contained:
 *      a. ASSIGN: 
 *      b. BREAK: Must be under CASE, FOR
 *      c. INCREMENT/DECREMENT
 *      d. include
 * 2. Tags with no end tag and covers everything from the tag to next tag and are recursive:
 *      a. WHEN: Should be in between case and endcase. Should be absord everything in between when and
 *         when, else, endcase.
 *      b. ELIF, ELSE:
 *      c. IF
 * 3. Tags with end tag:
 *      a. FOR
 *      b. CASE
 * 4. Tags with end tag that captures everything as text:
 *      a. RAW
 *      b. COMMENT
 *      c. CAPTURE
 *
 */

static pt_node_t *new_pt_node(pt_node_t *parent, enum pt_node_type type)
{
    pt_node_t *n;
    n = safe_calloc(1, sizeof(pt_node_t));
    n->type = type;
    n->parent = parent;
    return n;
}

pt_node_t *new_pt_branch(pt_node_t *parent, lexer_block_t *pblk)
{
    // TODO: Possibilities of having first token as operator.
    bool has_1_operand = pblk->tok.tag.tokens[0] != NULL;
    bool has_op = has_1_operand && (pblk->tok.tag.tokens[1] != NULL);
    bool has_2_operand = has_op && (pblk->tok.tag.tokens[2] != NULL);

    pt_node_t *if_node = new_pt_node(parent, PT_NODE_BRANCH);
    if (has_1_operand)
        if_node->branch.condition.lhs = safe_strdup(pblk->tok.tag.tokens[0]);
    if (has_op)
        if_node->branch.condition.operator = liquid_get_optor(pblk->tok.tag.tokens[1]);
    if (has_2_operand)
        if_node->branch.condition.rhs = safe_strdup(pblk->tok.tag.tokens[2]);
    if_node->liq_kw = pblk->tok.tag.keyword;
    return if_node;
}

pt_node_t *new_pt_loop(pt_node_t *parent, lexer_block_t *pblk)
{
    // TODO: Check if we have 3 valid tokens
    pt_node_t *for_node = new_pt_node(parent, PT_NODE_LOOP);
    for_node->branch.condition.lhs = safe_strdup(pblk->tok.tag.tokens[0]);
    for_node->branch.condition.operator = liquid_get_optor(pblk->tok.tag.tokens[1]);
    for_node->branch.condition.rhs = safe_strdup(pblk->tok.tag.tokens[2]);

    for_node->loop.variable = safe_malloc(sizeof(*for_node->loop.variable));
    for_node->loop.variable->identifier = pblk->tok.tag.tokens[0];
    for_node->loop.variable->expression = pblk->tok.tag.tokens[2];
    for_node->liq_kw = pblk->tok.tag.keyword;
    return for_node;
}

typedef struct {
    pt_node_t *root;
} parser_t;

bool is_tag_valid(pt_node_t *pcur_node, lexer_block_t *pblk)
{
    if ((pcur_node == NULL) || (pblk == NULL))
        return false;
    enum liq_kw kw = pblk->tok.tag.keyword;
    if (liquid_is_tag_enclosing(kw) && !liquid_is_block_end(kw))
        return true;

    // 1. If it is a end tag, then check if the tag has started in the past?
    //      Check if the tag that is ending has the corresponding start tag nearest
    if (liquid_is_block_end(kw)) {
        if (!pcur_node->parent)
            return false;
        return pcur_node->parent->liq_kw == liquid_get_start_tag(kw);
        
    }
    // Check for enclosed tags
    for (; pcur_node; pcur_node = pcur_node->parent) {

        // 2. If it is enclosed, it is being enclosed under right start tag (break, continue) at any depth.
        if (kw == LIQ_KW_BREAK || kw == LIQ_KW_CONTINUE) {
            if (pcur_node->liq_kw == LIQ_KW_FOR)
                return true;
            continue;
        }
        // 3. If it is enclosed, it is being enclosed just under right start tag (when, elseif, else)
        // if (kw == LIQ_KW_WHEN) {
        //     return pnode->parent->liq_kw == LIQ_
        // }
        if (kw == LIQ_KW_ELSIF) {
            if (pcur_node->liq_kw == LIQ_KW_IF)
                return true;
        }   
        if (kw == LIQ_KW_ELSE) {
            if (pcur_node->liq_kw == LIQ_KW_IF)
                return true;
            if (pcur_node->liq_kw == LIQ_KW_FOR)
                return true;
            if (pcur_node->liq_kw == LIQ_KW_CASE)
                return true;
        }   
    }
    return false;
}

pt_node_t *add_to_pt_tree(pt_node_t *pnode, pt_node_t **curr_node)
{
    if (*curr_node == NULL)
        *curr_node = pnode;
    else
        list_append(&((*curr_node)->children), &pnode->node);
    return pnode;
}

static pt_node_t *_build_parse_tree(parser_t *ctx, node_t **head, pt_node_t *parent_node)
{
    lexer_block_t *blk;
    pt_node_t *parse_node = NULL;
    // printf("Executing Function: %s\r\n", __func__);

    for (; *head != NULL; *head = (*head)->next) {
        blk = CONTAINER_OF(*head, lexer_block_t, node);
        // print_lex_block(blk);
        switch (blk->type) {
        case LEXER_BLOCK_DATA:
        {
            if (blk->content.len == 0)
                break;
            pt_node_t *pnode = new_pt_node(parent_node, PT_NODE_TEXT);
            add_to_pt_tree(pnode, &parse_node);
            pnode->liq_kw = blk->tok.tag.keyword;
            pnode->text.content = safe_strdup(blk->content.buf);
            break;
        }
        case LEXER_BLOCK_OBJECT:
        {
            pt_node_t *pnode = new_pt_node(parent_node, PT_NODE_OBJECT);
            pnode->liq_kw = blk->tok.tag.keyword;
            add_to_pt_tree(pnode, &parse_node);

            int num_filters = blk->tok.obj.num_filters;
            if (blk->tok.obj.identifier)
                pnode->object.identifier = safe_strdup(blk->tok.obj.identifier);
            if (!num_filters)
                break;
            pnode->object.filters = safe_calloc(num_filters, sizeof(liq_filter_t));
            memcpy(pnode->object.filters, blk->tok.obj.filters, num_filters * sizeof(liq_filter_t));
            pnode->object.num_filters = num_filters;
            break;
        }
        case LEXER_BLOCK_TAG:
        {
            enum liq_kw liq_type = blk->tok.tag.keyword;
            if (!is_tag_valid(parse_node, blk)) {
                printf("Invalid Syntax.\r\n");
                exit(-1);
            }
            switch (liq_type) {
            case LIQ_KW_IF:
            {
                pt_node_t *pnode = new_pt_branch(parent_node, blk);
                add_to_pt_tree(pnode, &parse_node);
                do {
                    *head = (*head)->next;
                    pnode->branch.true_body = _build_parse_tree(ctx, head, pnode);

                    blk = CONTAINER_OF(*head, lexer_block_t, node);
                    enum liq_kw kw = blk->tok.tag.keyword;
                    if (kw == LIQ_KW_ELSE) {
                        *head = (*head)->next;
                        pnode->branch.false_body = _build_parse_tree(ctx, head, pnode);
                        blk = CONTAINER_OF(*head, lexer_block_t, node);
                        if (blk->tok.tag.keyword != LIQ_KW_ENDIF) {
                            printf("Invalid Syntax: Expected ENDIF\r\n");
                            exit(-1);
                        }
                    } else if (kw == LIQ_KW_ELSIF) {
                        *head = (*head)->next;
                        pt_node_t *new_node = new_pt_branch(parent_node, blk);
                        pnode->branch.false_body = new_node;
                        pnode = new_node;
                        continue;
                    } else {
                        printf("Invalid Syntax: Expected ENDIF\r\n");
                        exit(-1);
                    }
                    break;
                } while (true);
                break;
            }
            case LIQ_KW_UNLESS:
            {
                pt_node_t *pnode = new_pt_branch(parent_node, blk);
                add_to_pt_tree(pnode, &parse_node);
                *head = (*head)->next;
                pnode->branch.true_body = _build_parse_tree(ctx, head, pnode);
                pnode->branch.false_body = NULL;
                blk = CONTAINER_OF(*head, lexer_block_t, node);
                liq_type = blk->tok.tag.keyword;
                if (liq_type != LIQ_KW_ENDFOR) {
                    printf("Error\r\n");
                    exit(-1);
                }
                *head = (*head)->next;
                break;
            }
            case LIQ_KW_CASE:
            {
                pt_node_t *pnode = new_pt_branch(parent_node, blk);
                add_to_pt_tree(pnode, &parse_node);
                const char * const identifier = safe_strdup(blk->tok.tag.tokens[0]);
                *head = (*head)->next;
                for (; *head != NULL; *head = (*head)->next) {
                    blk = CONTAINER_OF(*head, lexer_block_t, node);
                    if (blk->tok.tag.keyword == LIQ_KW_WHEN)
                        break;
                    if (blk->type == LEXER_BLOCK_DATA)
                        continue;
                    printf("Invalid Syntax. When should be after case\r\n");
                    exit(-1);
                }
                if (*head == NULL)
                    exit(-1);
                do {
                    blk = CONTAINER_OF(*head, lexer_block_t, node);
                    const char *to_equal = blk->tok.tag.tokens[0];
                    
                    pnode->branch.condition.lhs = safe_strdup(identifier);
                    pnode->branch.condition.operator = LIQ_OP_EQUAlS;
                    pnode->branch.condition.rhs = safe_strdup(to_equal);

                    *head = (*head)->next;
                    pnode->branch.true_body = _build_parse_tree(ctx, head, pnode);
                    blk = CONTAINER_OF(*head, lexer_block_t, node);
                    liq_type = blk->tok.tag.keyword;

                    if (liq_type == LIQ_KW_WHEN) {
                        pt_node_t *new_node = new_pt_node(parent_node, PT_NODE_BRANCH);
                        pnode->branch.false_body = new_node;
                        pnode = new_node;
                        continue;
                    } else if (liq_type == LIQ_KW_ELSE) {
                        *head = (*head)->next;
                        pnode->branch.false_body = _build_parse_tree(ctx, head, pnode);
                        blk = CONTAINER_OF(*head, lexer_block_t, node);
                        if (blk->tok.tag.keyword == LIQ_KW_ENDCASE)
                            break;
                    } else if (liq_type == LIQ_KW_ENDCASE) {
                        pnode->branch.false_body = NULL;
                        break;
                    }
                    printf("Invalid Syntax: Expected ENDCASE\r\n");
                    exit(-1);
                } while (true);
                break;
            }
            case LIQ_KW_FOR:
            {
                pt_node_t *pnode = new_pt_loop(parent_node, blk);
                add_to_pt_tree(pnode, &parse_node);
                *head = (*head)->next;
                pnode->loop.body = _build_parse_tree(ctx, head, pnode);
                blk = CONTAINER_OF(*head, lexer_block_t, node);
                // TODO: Check for else
                if (blk->tok.tag.keyword != LIQ_KW_ENDFOR) {
                    printf("Error\r\n");
                    exit(-1);
                }
                *head = (*head)->next;
                break;
            }
            case LIQ_KW_BREAK:
            case LIQ_KW_CONTINUE:
            {
                pt_node_t *pnode = new_pt_node(parent_node, PT_NOTE_STMT);
                pnode->statement.keyword = liq_type;
                pnode->liq_kw = liq_type;
                add_to_pt_tree(pnode, &parse_node);
                break;
            }
            case LIQ_KW_ASSIGN:
            {
                pt_node_t *pnode = new_pt_node(parent_node, PT_NODE_ASSIGN);
                pnode->liq_kw = liq_type;
                add_to_pt_tree(pnode, &parse_node);

                pnode->assign.identifier = safe_strdup(blk->tok.tag.tokens[0]);
                // TODO: Add others
                break;
            }
            case LIQ_KW_INCREMENT:
            {
                pt_node_t *pnode = new_pt_node(parent_node, PT_NODE_ASSIGN);
                pnode->liq_kw = liq_type;
                add_to_pt_tree(pnode, &parse_node);

                pnode->assign.identifier = safe_strdup(blk->tok.tag.tokens[0]);
                // TODO: Add others
                break;
            }
            case LIQ_KW_DECREMENT:
            {
                pt_node_t *pnode = new_pt_node(parent_node, PT_NODE_ASSIGN);
                pnode->liq_kw = liq_type;
                add_to_pt_tree(pnode, &parse_node);

                pnode->assign.identifier = safe_strdup(blk->tok.tag.tokens[0]);
                // TODO: Add others
                break;
            }
            case LIQ_KW_WHEN:
            case LIQ_KW_ELSIF:
            case LIQ_KW_ELSE:
            {
                return parse_node;
            }
            case LIQ_KW_ENDIF:
            case LIQ_KW_ENDCAPTURE:
            case LIQ_KW_ENDCASE:
            case LIQ_KW_ENDCOMMENT:
            case LIQ_KW_ENDFOR:
            case LIQ_KW_ENDRAW:
            case LIQ_KW_ENDUNLESS:
            {
                return parse_node;
            }
            default:
            {

                break;
            }
            }
        }
        default:
        {
            // Raise an error
            break;
        }
        }
    }
    return parse_node;
}

int build_parse_tree(parser_t *ctx, list_t *lex_blocks)
{
    node_t *head = lex_blocks->head;
    ctx->root = _build_parse_tree(ctx, &head, NULL);
    if (ctx->root)
        return 0;
    return -1;
}

void parser_setup(fluid_t *ctx)
{
    parser_t *p;

    p = safe_malloc(sizeof(parser_t));
    p->root = NULL;

    ctx->parser_data = p;
}

int parser_parse(fluid_t *ctx)
{
    parser_t *p = ctx->parser_data;

    if (build_parse_tree(p, &ctx->lex_blocks))
        return -1;
    return 0;
}

void parser_teardown(fluid_t *ctx)
{
    parser_t *p = ctx->parser_data;
    safe_free(p);
}
