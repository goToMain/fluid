/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stddef.h>
#include <stdbool.h>

#include <utils/utils.h>
#include <utils/list.h>
#include <utils/hashmap.h>

#include "ferrors.h"

/**
 * @brief Fluid primitive types.
 */

enum fluid_ptype_e {
    FLUID_PTYPE_NIL,
    FLUID_PTYPE_NUMBER,
    FLUID_PTYPE_STRING,
    FLUID_PTYPE_BOOLEAN,
};

typedef struct  {
    double data;
} fluid_ptype_number_t;

typedef struct {
    char *data;
    size_t length;
} fluid_ptype_string_t;

typedef struct {
    bool data;
} fluid_ptype_boolean_t;

typedef struct {
    node_t node;
    enum fluid_ptype_e type;
    union {
        fluid_ptype_number_t number;
        fluid_ptype_string_t string;
        fluid_ptype_boolean_t boolean;
    };
} fluid_ptype_t;

/**
 * @brief Fluid composite types.
 */

#define FLUID_OBJECT_IDENTIFIER_MAXLEN 32

enum fluid_type_e {
    FLUID_TYPE_PRIMITIVE,
    FLUID_TYPE_LIST,
    FLUID_TYPE_CONTAINER,
};

typedef struct {
    list_t items;
    size_t length;
    enum fluid_type_e type;
} fluid_list_t;

typedef struct fluid_object_s fluid_object_t;

struct fluid_object_s {
    char identifer[FLUID_OBJECT_IDENTIFIER_MAXLEN];
    enum fluid_type_e type;
    union {
        fluid_ptype_t ptype;
        hash_map_t members;
        fluid_list_t list;
    };
    fluid_object_t *parent;
    node_t node;
};

void fluid_object_dump(fluid_object_t *obj);
ferror_t fluid_object_new(const char *identifier, fluid_object_t *parent,
                          fluid_object_t **object);
ferror_t fluid_object_add_value(fluid_object_t *obj, const char *value);
ferror_t fluid_object_nest(fluid_object_t *parent, fluid_object_t *child);
ferror_t fluid_object_cast_containier(fluid_object_t *obj);
ferror_t fluid_object_cast_list(fluid_object_t *obj);
ferror_t fluid_object_list_append(fluid_object_t *list_obj, fluid_object_t *item);
ferror_t fluid_object_delete(fluid_object_t *obj);

#endif /* _TYPES_H_ */
