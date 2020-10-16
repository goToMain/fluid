/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#include <utils/utils.h>
#include <utils/hashmap.h>

typedef struct ftype_number {
    double data;
} ftype_number_t;

typedef struct ftype_string {
    char *data;
    size_t length;
} ftype_string_t;

typedef struct ftype_boolean {
    bool data;
} ftype_boolean_t;

typedef struct ftype_list {
    void **items;
    size_t capacity;
    size_t length;
} ftype_list_t;

typedef struct ftype_dict {
    hash_map_t map;
    size_t count;
} ftype_dict_t;

enum ftype_e {
    FTYPE_NIL,
    FTYPE_NUMBER,
    FTYPE_STRING,
    FTYPE_BOOLEAN,
    FTYPE_LIST,
    FTYPE_DICT,
};

typedef struct fobject {
    enum ftype_e type;
    union {
        ftype_number_t number;
        ftype_string_t string;
        ftype_boolean_t boolean;
        ftype_list_t list;
        ftype_dict_t dict;
    };
    int ref_count;
} fobject_t;

/* --- Begin PRIVATE --- */

fobject_t *__fobj_new(enum ftype_e type);
void *__fobj_delete(fobject_t *obj);

#define _INC_REF(obj) ({                                         \
        (obj)->ref_count++;                                      \
        obj;                                                     \
    })

#define _DEC_REF(obj) ({                                         \
        assert((obj)->ref_count > 0);                            \
        (--((obj)->ref_count) == 0) ? __fobj_delete(obj) : obj;  \
    })

/* --- End PRIVATE --- */

#define INC_REF(obj) (obj) ? _INC_REF((fobject_t *)(obj)) : NULL;
#define DEC_REF(obj) (obj) ? _DEC_REF((fobject_t *)(obj)) : NULL;

/* ------------------------------- */
/*           Primitive             */
/* ------------------------------- */

fobject_t *fobj_from_double(double val);
fobject_t *fobj_from_cstring(const char *val);
fobject_t *fobj_from_bool(bool val);
int fobj_to_double(fobject_t *obj, double *val);
int fobj_to_cstring(fobject_t *obj, char **val, int *len);
int fobj_to_bool(fobject_t *obj, bool *val);
int fobj_autovivify(fobject_t **obj, const char *literal);

/* ------------------------------- */
/*              List               */
/* ------------------------------- */

fobject_t *flist_new(size_t size);
size_t flist_length(fobject_t *obj);
int flist_get_item(fobject_t *obj, size_t offset, fobject_t **item);
int flist_set_item(fobject_t *obj, size_t offset, fobject_t *item);
void flist_grow(fobject_t *obj);
int flist_insert(fobject_t *obj, size_t offset, fobject_t *item);
int flist_remove(fobject_t *obj, size_t offset, fobject_t **item);
int flist_append(fobject_t *obj, fobject_t *item);

/* ------------------------------- */
/*           Dictionary            */
/* ------------------------------- */

fobject_t *fdict_new();
fobject_t *fdict_get_item(fobject_t *obj, const char *key);
int fdict_insert_item(fobject_t *obj, const char *key, fobject_t *item);
fobject_t *fdict_delete_item(fobject_t *obj, const char *key);

#endif /* _TYPES_H_ */
