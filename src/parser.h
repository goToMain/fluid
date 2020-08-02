/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PARSER_H_
#define _PARSER_H_

#include "fluid.h"

void parser_setup(fluid_t *ctx);
int parser_parse(fluid_t *lex);
void parseer_teardown(fluid_t *ctx);

#endif /* _PARSER_H_ */
