/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "objects.h"

/*

Idea Sketch:

my_var.bar = 5
    bar::FLUID_PTYPE_NUMBER
    my_var::FLUID_TYPE_OBJECT

my_var.foo = "string"
    foo::FLUID_PTYPE_STRING
    my_var::FLUID_TYPE_OBJECT

my_var.baz = [ "This", "is", "a", "list" ]
    baz::FLUID_TYPE_LIST
    my_var::FLUID_TYPE_OBJECT

    my_var.baz[3]
        baz[3]::FLUID_PTYPE_STRING
        baz::FLUID_TYPE_LIST
        my_var::FLUID_TYPE_OBJECT

*/

#define FLUID_TYPE_OK                0
#define FLUID_TYPE_ERR_UNKNOWN      -1
#define FLUID_TYPE_ERR_NUM          -2
#define FLUID_TYPE_ERR_STR          -3

#define IS_STRING_ISH(s) (*(s) == '"' || *(s) == '\'')
#define IS_NUMBER_ISH(s) (*(s) == '+' || *(s) == '-' || *(s) == '.' || isdigit(*(s)))

ferror_t fluid_autovivify_primitive(const char *literal, fluid_ptype_t *p)
{
    int i;
    char *end, len;

    p->type = FLUID_PTYPE_NIL;
    len = strlen(literal);

    if (len == 0)
        fexcept(FERROR_OBJECT_VAL_LITERAL);

    if (IS_NUMBER_ISH(literal)) {
        p->type = FLUID_PTYPE_NUMBER;
        p->number.data = strtod(literal, &end);
        if (*end != '\0')
            return FLUID_TYPE_ERR_NUM;
    }
    else if (strcmp(literal, "true") == 0) {
        p->type = FLUID_PTYPE_BOOLEAN;
        p->boolean.data = true;
    }
    else if (strcmp(literal, "false") == 0 ) {
        p->type = FLUID_PTYPE_BOOLEAN;
        p->boolean.data = false;
    }
    else if (IS_STRING_ISH(literal)) {
        i = 0;
        while (literal[i] && !IS_STRING_ISH(literal + i))
            i++;
        if (!IS_STRING_ISH(literal + i) || literal[i + 1] != '\0')
            return FLUID_TYPE_ERR_STR;
        p->type = FLUID_PTYPE_STRING;
        p->string.data = safe_strdup(literal + 1);
        p->string.data[i] = '\0';
        p->string.length = i;
    }
    else {
        p->type = FLUID_PTYPE_STRING;
        p->string.data = safe_strdup(literal);
        p->string.length = len;
    }

    return FERROR_OK;
}

ferror_t fluid_object_new(const char *identifier, fluid_object_t **object)
{
    int length;
    fluid_object_t *obj;

    if (identifier) {
        length = strlen(identifier);
        if (length >= FLUID_OBJECT_IDENTIFIER_MAXLEN)
            fexcept(FERROR_OBJECT_IDENTIFIER);
    }

    obj = safe_calloc(1, sizeof(fluid_object_t));
    if (identifier)
        strcpy(obj->identifer, identifier);
    obj->type = FLUID_TYPE_UNDEF;
    *object = obj;

    return FERROR_OK;
}

ferror_t fluid_object_add_value(fluid_object_t *obj, const char *value)
{
    ferror_t e;

    e = fluid_autovivify_primitive(value, &obj->ptype);
    fexcept_proagate(e);
    obj->type = FLUID_TYPE_PRIMITIVE;

    return FERROR_OK;
}

void fluid_object_nest(fluid_object_t *parent, fluid_object_t *child)
{
    switch (parent->type) {
    case FLUID_TYPE_UNDEF:
        parent->type = FLUID_TYPE_OBJECT;
        /* FALLTHRU */
    case FLUID_TYPE_OBJECT:
        hash_map_insert(&parent->members, child->identifer, child);
        break;
    case FLUID_TYPE_LIST:
        list_append(&parent->list.items, &child->node);
        break;
    default:
        break;
    }
}

/* --- lists --- */

void fluid_object_cast_list(fluid_object_t *obj)
{
    obj->type = FLUID_TYPE_LIST;
    list_init(&obj->list.items);
}

ferror_t fluid_object_list_append(fluid_object_t *list_obj, fluid_object_t *item)
{
    if (list_obj->type != FLUID_TYPE_LIST)
        fexcept(FERROR_OBJECT_TYPE);

    list_append(&list_obj->list.items, &item->node);

    return FERROR_OK;
}

/* --- distructors --- */

ferror_t fluid_object_delete(fluid_object_t *obj)
{
    if (obj == NULL)
        fexcept(FERROR_INVALID_PARAM);

    switch (obj->type) {
    case FLUID_TYPE_UNDEF:
    case FLUID_TYPE_PRIMITIVE:
        break;
    case FLUID_TYPE_LIST:
        break;
    case FLUID_TYPE_OBJECT:
        /* get iterator over members */
        /* recurse fluid_delete_object(member) */
        break;
    default:
        break;
    }
    safe_free(obj);

    return FERROR_OK;
}
