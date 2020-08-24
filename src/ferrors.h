/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FERROR_H_
#define _FERROR_H_

#include <stdio.h>

/**
 * @brief fluid exception handling mechanism
 */

typedef enum ferror_e {

    FERROR_OK, /* all okay, no errors */

    /* generic errors */
    FERROR_GE_START,
        FERROR_UNKNOWN,
        FERROR_FILE_NOT_FOUND,
        FERROR_INVALID_PARAM,
    FERROR_GE_END,

    /* object errors */
    FERROR_OE_START,
        FERROR_OBJECT_VAL_LITERAL,
        FERROR_OBJECT_IDENTIFIER,
        FERROR_OBJECT_TYPE,
    FERROR_OE_END,

    /* config errors */
    FERROR_CE_START,
        FERROR_CONFIG_PARSER,
        FERROR_CONFIG_EVENT,
    FERROR_CE_END,

} ferror_t;

const char *ferror_type(ferror_t e);
const char *ferror_message(ferror_t e);

#define fexcept(e)                                                       \
    do {                                                                 \
        fprintf(stderr, "EXCEPTION: %s at %s - %s",                      \
                ferror_type(e), __FUNCTION__, ferror_message(e));        \
        return e;                                                        \
    } while(0)

#define fexcept_goto(e, l)                                               \
    do {                                                                 \
        fprintf(stderr, "EXCEPTION: %s at %s - %s",                      \
                ferror_type(e), __FUNCTION__, ferror_message(e));        \
        goto l;                                                          \
    } while(0)

#define fexcept_proagate(e)                                              \
    do {                                                                 \
        if (e != FERROR_OK)                                              \
            return e;                                                    \
    } while (0)

#define fexcept_proagate_goto(e, l)                                      \
    do {                                                                 \
        if (e != FERROR_OK)                                              \
            goto l;                                                      \
    } while (0)

#endif  /* _FERROR_H_ */
