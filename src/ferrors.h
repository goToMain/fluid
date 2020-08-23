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
    FERROR_OK,

    // Generic errors
    FERROR_UNKNOWN,
    FERROR_FILE_NOT_FOUND,
    FERROR_INVALID_PARAM,

    // Object errors
    FERROR_OBJECT_VAL_LITERAL,
    FERROR_OBJECT_IDENTIFIER,
    FERROR_OBJECT_TYPE,

    // Config errors
    FERROR_CONFIG_PARSER,
    FERROR_CONFIG_EVENT,

    FERROR_SENTINEL
} ferror_t;

const char *ferror_type(ferror_t e);
const char *ferror_message(ferror_t e);

#define fexcept(e) \
    do { \
        fprintf(stderr, "EXCEPTION: %s at %s - %s", \
                ferror_type(e), __FUNCTION__, ferror_message(e)); \
        return e; \
    } while(0)

#define fexcept_goto_error(e) \
    do { \
        fprintf(stderr, "EXCEPTION: %s at %s - %s", \
                ferror_type(e), __FUNCTION__, ferror_message(e)); \
        goto error; \
    } while(0)

#define fexcept_proagate(e) \
    do { \
        if (e != FERROR_OK) \
            return e;  \
    } while (0)

#define fexcept_proagate_goto_error(e) \
    do { \
        if (e != FERROR_OK) \
            goto error;  \
    } while (0)

#endif  /* _FERROR_H_ */
