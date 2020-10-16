/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "fobjects.h"

/*

Idea Sketch:

my_var.bar = 5
    bar::FTYPE_NUMBER
    my_var::FTYPE_DICT

my_var.foo = "string"
    foo::FTYPE_STRING
    my_var::FTYPE_DICT

my_var.baz = [ "This", "is", "a", "list" ]
    baz::FTYPE_LIST
    my_var::FTYPE_DICT

    my_var.baz[3]
        baz[3]::FTYPE_STRING
        baz::FTYPE_LIST
        my_var::FTYPE_DICT

*/

#define FTYPE_OK                0
#define FTYPE_ERR_UNKNOWN      -1
#define FTYPE_ERR_NUM          -2
#define FTYPE_ERR_STR          -3

#define IS_STRING_ISH(s) (*(s) == '"' || *(s) == '\'')
#define IS_NUMBER_ISH(s) (*(s) == '+' || *(s) == '-' || *(s) == '.' || isdigit(*(s)))

fobject_t *__fobj_new(enum ftype_e type)
{
    int length;
    fobject_t *obj;

    obj = safe_calloc(1, sizeof(fobject_t));
    obj->type = type;

    return INC_REF(obj); /* ref count is 1 for new objects  */
}

void *__fobj_delete(fobject_t *obj)
{
    char *identifier;
    fobject_t *tmp;
    hash_map_iterator_t it;

    assert(obj->ref_count == 1);

    switch (obj->type) {
    case FTYPE_LIST:
        safe_free(obj->list.items);
        break;
    case FTYPE_DICT:
        hash_map_it_init(&it, &obj->dict.map);
        while (hash_map_it_next(&it, &identifier, &tmp) == 0) {
            DEC_REF(tmp);
            hash_map_delete(&obj->dict.map, identifier, 0);
        }
        break;
    default:
        break;
    }
    free(obj);
    return NULL;
}

/* ------------------------------- */
/*           Primitive             */
/* ------------------------------- */

fobject_t *fobj_from_double(double val)
{
    fobject_t *obj;

    obj = __fobj_new(FTYPE_NUMBER);
    obj->number.data = val;
    return obj;
}

fobject_t *fobj_from_cstring(const char *val)
{
    fobject_t *obj;

    obj = __fobj_new(FTYPE_STRING);
    obj->string.data = safe_strdup(val);
    obj->string.length = strlen(val);
    return obj;
}

fobject_t *fobj_from_bool(bool val)
{
    fobject_t *obj;

    obj = __fobj_new(FTYPE_BOOLEAN);
    obj->boolean.data = val;
    return obj;
}


int fobj_to_double(fobject_t *obj, double *val)
{
    if (obj->type != FTYPE_NUMBER)
        return -1;

    *val = obj->number.data;
    return 0;
}

int fobj_to_cstring(fobject_t *obj, char **val, int *len)
{
    if (obj->type != FTYPE_STRING)
        return -1;

    *val = obj->string.data;
    if (len)
        *len = obj->string.length;
    return 0;
}

int fobj_to_bool(fobject_t *obj, bool *val)
{
    if (obj->type != FTYPE_BOOLEAN)
        return -1;

    *val = obj->boolean.data;
    return 0;
}

int fobj_autovivify(fobject_t **obj, const char *literal)
{
    int i;
    double val_double;
    char *tmp, len;

    len = strlen(literal);

    if (len == 0)
        return -1;

    if (IS_NUMBER_ISH(literal)) {
        val_double = strtod(literal, &tmp);
        if (*tmp != '\0')
            fexcept(FTYPE_ERR_NUM);
        *obj = fobject_from_double(val_double);
    }
    else if (strcmp(literal, "true") == 0) {
        *obj = fobject_from_bool(true);
    }
    else if (strcmp(literal, "false") == 0 ) {
        *obj = fobject_from_bool(false);
    }
    else if (IS_STRING_ISH(literal)) {
        i = 1;
        while (literal[i] && !IS_STRING_ISH(literal + i))
            i++;
        if (!IS_STRING_ISH(literal + i) || literal[i + 1] != '\0')
            fexcept(FTYPE_ERR_STR);
        tmp = safe_strdup(literal + 1);
        tmp[i-1] = '\0';
        *obj = fobject_from_cstring(tmp);
    }
    else {
        *obj = fobject_from_cstring(literal);
    }

    return 0;
}

/* ------------------------------- */
/*              List               */
/* ------------------------------- */

fobject_t *flist_new(size_t size)
{
    fobject_t *obj;

    obj = __fobj_new(FTYPE_LIST);
    if (size > 0) {
        obj->list.items = safe_calloc(size, sizeof(fobject_t *));
        obj->list.capacity = size;
    }
    return obj;
}

size_t flist_length(fobject_t *obj)
{
    return obj->list.length;
}

int flist_get_item(fobject_t *obj, size_t offset, fobject_t **item)
{
    if (obj->type != FTYPE_LIST)
        return -1;

    if (offset >= obj->list.length)
        return -2;

    *item = obj->list.items[offset];

    return 0;
}

int flist_set_item(fobject_t *obj, size_t offset, fobject_t *item)
{
    fobject_t *old;

    if (obj->type != FTYPE_LIST)
        return -1;
    if (offset >= obj->list.length)
        return -2;
    DEC_REF(obj->list.items[offset]);
    obj->list.items[offset] = INC_REF(item);
    return 0;
}

void flist_grow(fobject_t *obj)
{
    size_t new_capacity;

    new_capacity = obj->list.capacity * 2;
    obj->list.items = safe_realloc_zero(obj->list.items,
                                        obj->list.capacity, new_capacity);
    obj->list.capacity = new_capacity;
}

int flist_insert(fobject_t *obj, size_t offset, fobject_t *item)
{
    size_t pos;

    if (obj->type != FTYPE_LIST)
        return -1;

    if (offset >= obj->list.length)
        return flist_append(obj, item);

    if (obj->list.length + 1 >= obj->list.capacity)
        flist_grow(obj);

    pos = obj->list.length + 1;
    while (pos > offset) {
        obj->list.items[pos] = obj->list.items[pos - 1];
        pos--;
    }
    obj->list.items[offset] = INC_REF(item);
    obj->list.length++;
    return 0;
}

int flist_remove(fobject_t *obj, size_t offset, fobject_t **item)
{
    fobject_t *val;

    if (obj->type != FTYPE_LIST)
        return -1;
    if (offset >= obj->list.length)
        return -2;

    val = DEC_REF(obj->list.items[offset]);
    while (offset < (obj->list.length - 1)) {
        obj->list.items[offset] = obj->list.items[offset + 1];
        offset++;
    }
    obj->list.length--;
    if (item)
        *item = val;
    return 0;
}

int flist_append(fobject_t *obj, fobject_t *item)
{
    if (obj->type != FTYPE_LIST)
        return -1;
    if (obj->list.length + 1 >= obj->list.capacity)
        flist_grow(obj);
    obj->list.items[obj->list.length] = INC_REF(item);
    obj->list.length++;
    return 0;
}

/* ------------------------------- */
/*           Dictionary            */
/* ------------------------------- */

fobject_t *fdict_new()
{
    fobject_t *obj;

    obj = __fobj_new(FTYPE_DICT);
    hash_map_init(&obj->dict.map);

    return obj;
}

fobject_t *fdict_get_item(fobject_t *obj, const char *key)
{
    if (obj->type != FTYPE_DICT)
        return -1;

    return hash_map_get(&obj->dict.map, key, 0);
}

int fdict_insert_item(fobject_t *obj, const char *key, fobject_t *item)
{
    if (obj->type != FTYPE_DICT)
        return -1;

    hash_map_insert(&obj->dict.map, key, INC_REF(item));
    obj->dict.count++;

    return 0;
}

fobject_t *fdict_delete_item(fobject_t *obj, const char *key)
{
    fobject_t *item;

    if (obj->type != FTYPE_DICT)
        return NULL;

    item = hash_map_delete(&obj->dict.map, key, 0);
    obj->dict.count--;

    return DEC_REF(item);
}
