/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ferrors.h"

const char *ferror_type(ferror_t e)
{
    if (e >= FERROR_CONFIG_PARSER && e <= FERROR_CONFIG_PARSER)
        return "ConfigError";

    if (e >= FERROR_OBJECT_VAL_LITERAL && e <= FERROR_OBJECT_TYPE)
        return "ObjectError";

    return "GenericError";
}

const char *ferror_message(ferror_t e)
{
    switch(e) {
    case FERROR_UNKNOWN:
        return "unknown error";
    case FERROR_FILE_NOT_FOUND:
        return "file not found";
    case FERROR_INVALID_PARAM:
        return "invalid parameter";

    // Object errors
    case FERROR_OBJECT_VAL_LITERAL:
        return "invalid value literal";
    case FERROR_OBJECT_IDENTIFIER:
        return "object identifer maxlen exceeded";
    case FERROR_OBJECT_TYPE:
        return "invalid object type";

    // Config errors
    case FERROR_CONFIG_PARSER:
        return "yaml parser error";
    case FERROR_CONFIG_EVENT:
        return "invalid yaml event";

    case FERROR_OK:
    case FERROR_SENTINEL:
        break;
    }
    return "";
}
