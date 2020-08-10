/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FILTER_H_
#define _FILTER_H_

#define LIQ_FILTER_ARG_MAXLEN   32
#define LIQ_FILTER_ARG_COUNT    2

enum liq_filter {
    LIQ_FILTER_NONE,
    LIQ_FILTER_STRIP,
    LIQ_FILTER_LSTRIP,
    LIQ_FILTER_RSTRIP,
    LIQ_FILTER_SENTINEL,
};

typedef struct {
    enum liq_filter id;
    char args[LIQ_FILTER_ARG_COUNT][LIQ_FILTER_ARG_MAXLEN + 1];
} liq_filter_t;

enum liq_filter get_filter_id(const char *identifer);
int filter_execute(liq_filter_t *f, char *in);
int liq_filter_arg_count(enum liq_filter id);

#endif  /* _FILTER_H_ */
