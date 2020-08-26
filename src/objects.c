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
    my_var::FLUID_TYPE_CONTAINER

my_var.foo = "string"
    foo::FLUID_PTYPE_STRING
    my_var::FLUID_TYPE_CONTAINER

my_var.baz = [ "This", "is", "a", "list" ]
    baz::FLUID_TYPE_LIST
    my_var::FLUID_TYPE_CONTAINER

    my_var.baz[3]
        baz[3]::FLUID_PTYPE_STRING
        baz::FLUID_TYPE_LIST
        my_var::FLUID_TYPE_CONTAINER

*/

#define STRING_FALSE(s)  (s == NULL || *s == '\0')
#define IS_STRING_ISH(s) (*(s) == '"' || *(s) == '\'')
#define IS_NUMBER_ISH(s) (*(s) == '+' || *(s) == '-' || *(s) == '.' || isdigit(*(s)))
#define PTYPE_CHECK(o, t) (o->type != FLUID_TYPE_PRIMITIVE || o->ptype.type != t)

const char *object_type_name(fluid_object_t *obj)
{
    switch(obj->type) {
        case FLUID_TYPE_PRIMITIVE:
            switch (obj->ptype.type) {
            case FLUID_PTYPE_NIL:       return "P:NIL";
            case FLUID_PTYPE_NUMBER:    return "P:NUMBER";
            case FLUID_PTYPE_STRING:    return "P:STRING";
            case FLUID_PTYPE_BOOLEAN:   return "P:BOOL";
            }
            return "P:error";
        case FLUID_TYPE_LIST:           return "C:LIST";
        case FLUID_TYPE_CONTAINER:      return "C:CONTAINER";
    }
    return "error";
}

void print_ptype_value(fluid_ptype_t *p)
{
    switch (p->type) {
    case FLUID_PTYPE_NIL:
        printf("NIL");
        break;
    case FLUID_PTYPE_NUMBER:
        printf("%f", p->number.data);
        break;
    case FLUID_PTYPE_STRING:
        printf("%s", p->string.data);
        break;
    case FLUID_PTYPE_BOOLEAN:
        printf("%s", p->boolean.data ? "true" : "false");
        break;
    }
}

static void
fluid_object_dump_helper(fluid_object_t *obj, int level, const char *prefix)
{
    int i;
    char *key;
    fluid_object_t *p;

    for(i = 0; i < level; i++)
        printf("  ");
    if (prefix)
        printf("%s", prefix);
    printf("%s [%s]: ", object_type_name(obj), obj->identifer);

    switch(obj->type) {
    case FLUID_TYPE_PRIMITIVE:
        print_ptype_value(&obj->ptype);
        printf("\n");
        break;
    case FLUID_TYPE_LIST:
        printf("\n");
        LIST_FOREACH(&obj->list.items, node) {
            p = CONTAINER_OF(node, fluid_object_t, node);
            fluid_object_dump_helper(p, level + 1, " - ");
        }
        break;
    case FLUID_TYPE_CONTAINER: {
        printf("\n");
        HASH_MAP_FOREACH(&obj->members, &key, &p) {
            fluid_object_dump_helper(p, level + 1, "");
        }
    }
        break;
    }
}

void fluid_object_dump(fluid_object_t *obj)
{
    fluid_object_dump_helper(obj, 0, NULL);
}

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
            fexcept(FERROR_OBJECT_TYPE);
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
            fexcept(FERROR_OBJECT_TYPE);
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

ferror_t fluid_object_new(const char *identifier, fluid_object_t *parent,
                          fluid_object_t **object)
{
    size_t length;
    fluid_object_t *obj;

    if (identifier) {
        length = strlen(identifier);
        if (length >= FLUID_OBJECT_IDENTIFIER_MAXLEN)
            fexcept(FERROR_OBJECT_IDENTIFIER);
    }

    obj = safe_calloc(1, sizeof(fluid_object_t));
    if (identifier)
        strcpy(obj->identifer, identifier);
    obj->type = FLUID_TYPE_PRIMITIVE;
    obj->ptype.type = FLUID_PTYPE_NIL;
    obj->parent = parent;
    *object = obj;

    return FERROR_OK;
}

ferror_t fluid_object_cast_containier(fluid_object_t *obj)
{
    if (PTYPE_CHECK(obj, FLUID_PTYPE_NIL))
        fexcept(FERROR_OBJECT_TYPE);

    obj->type = FLUID_TYPE_CONTAINER;
    hash_map_init(&obj->members);

    return FERROR_OK;
}

ferror_t fluid_object_add_value(fluid_object_t *obj, const char *value)
{
    FEX( fluid_autovivify_primitive(value, &obj->ptype) );
    obj->type = FLUID_TYPE_PRIMITIVE;

    return FERROR_OK;
}

ferror_t fluid_object_nest(fluid_object_t *parent, fluid_object_t *child)
{
    if (parent == NULL || child == NULL)
        fexcept_printf(FERROR_INVALID_PARAM, "parent: %p child: %p", parent, child);

    switch (parent->type) {
    case FLUID_TYPE_CONTAINER:
        if (STRING_FALSE(child->identifer))
            fexcept(FERROR_OBJECT_IDENTIFIER);
        hash_map_insert(&parent->members, child->identifer, child);
        break;
    case FLUID_TYPE_LIST:
        list_append(&parent->list.items, &child->node);
        break;
    default: fexcept(FERROR_OBJECT_TYPE);
    }

    return FERROR_OK;
}

/* --- lists --- */

ferror_t fluid_object_cast_list(fluid_object_t *obj)
{
    if (PTYPE_CHECK(obj, FLUID_PTYPE_NIL))
        fexcept(FERROR_OBJECT_TYPE);

    obj->type = FLUID_TYPE_LIST;
    list_init(&obj->list.items);

    return FERROR_OK;
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
    case FLUID_TYPE_PRIMITIVE:
        break;
    case FLUID_TYPE_LIST:
        break;
    case FLUID_TYPE_CONTAINER:
        /* get iterator over members */
        /* recurse fluid_delete_object(member) */
        break;
    default:
        break;
    }
    safe_free(obj);

    return FERROR_OK;
}
