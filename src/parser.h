/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PARSER_H_
#define _PARSER_H_

#include "fluid.h"
#include "lexer.h"
#include "liquid.h"
#include "filter.h"

enum pt_node_type {
    PT_NODE_TEXT,
    PT_NODE_OBJECT,
    PT_NOTE_STMT,
    PT_NODE_COMPARE,
    PT_NODE_ASSIGN,
    PT_NODE_BRANCH,
    PT_NODE_LOOP,
    PT_NODE_CONST,
    PT_NODE_SENTINEL
};

struct pt_node_text {
    char *content;
};

struct pt_node_object {
    char *identifier;
    liq_filter_t *filters;
    int num_filters;
};

struct pt_node_statement {
    enum liq_kw keyword;
};

struct pt_node_assign {
    char *identifier;
    char *expression;
    /** TODO: Must have a liq_const_t *default; */
};

struct pt_node_compare {
    char *lhs;
    enum liq_operators operator;
    char *rhs;
};

struct pt_node_branch {
    struct pt_node_compare condition;
    struct pt_node *true_body;
    struct pt_node *false_body;
};

struct pt_node_loop {
    struct pt_node_compare condition;
    struct pt_node_assign *variable;
    struct pt_node *body;
};

struct pt_node {
    node_t node;
    enum pt_node_type type;
    enum liq_kw liq_kw;
    union {
        struct pt_node_branch branch;
        struct pt_node_loop loop;
        struct pt_node_text text;
        struct pt_node_assign assign;
        struct pt_node_compare compare;
        struct pt_node_object object;
        struct pt_node_statement statement;
    };
    list_t children;
    struct pt_node *parent;
};

typedef struct pt_node pt_node_t;

void parser_setup(fluid_t *ctx);
int parser_parse(fluid_t *lex);
void parser_teardown(fluid_t *ctx);

#endif /* _PARSER_H_ */
