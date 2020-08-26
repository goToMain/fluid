/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ferrors.h"

const char *ferror_type(ferror_t e)
{
    if (e > FERROR_GE_START && e < FERROR_GE_END)
        return "GenericError";

    if (e > FERROR_OE_START && e < FERROR_OE_END)
        return "ObjectError";

    if (e > FERROR_CE_START && e < FERROR_CE_END)
        return "ConfigError";

    return "";
}

const char *ferror_message(ferror_t e)
{
    switch(e) {

    /* Generic errors */
    case FERROR_UNKNOWN:               return "unknown error";
    case FERROR_FILE_NOT_FOUND:        return "file not found";
    case FERROR_INVALID_PARAM:         return "invalid parameter";

    /* Object errors */
    case FERROR_OBJECT_VAL_LITERAL:    return "invalid value literal";
    case FERROR_OBJECT_IDENTIFIER:     return "object identifer maxlen exceeded";
    case FERROR_OBJECT_TYPE:           return "invalid object type";

    /* Config errors */
    case FERROR_CONFIG_PARSER:         return "yaml parser error";
    case FERROR_CONFIG_EVENT:          return "invalid yaml event";
    case FERROR_CONFIG_NESTING:        return "invalid object nesting request";

    /* lables that dont have a case */
    case FERROR_OK:
    case FERROR_GE_START:      case FERROR_GE_END:
    case FERROR_OE_START:      case FERROR_OE_END:
    case FERROR_CE_START:      case FERROR_CE_END:
        break;

    /**
     * Don't add a default case so the compiler can warn about that unhandled
     * cases in the switch.
     */
    }
    return "";
}
