/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <yaml.h>
#include <utils/file.h>

#include "objects.h"
#include "ferrors.h"

/**
 * LibYAML Grammer:
 *
 *    stream ::= STREAM-START document* STREAM-END
 *    document ::= DOCUMENT-START section* DOCUMENT-END
 *    section ::= MAPPING-START (key list) MAPPING-END
 *    list ::= SEQUENCE-START values* SEQUENCE-END
 *    values ::= MAPPING-START (key value)* MAPPING-END
 *    key = SCALAR
 *    value = SCALAR
 */

enum yaml_reader_state_e {
    YRS_START,
    YRS_OBJ_NEW,
    YRS_OBJ_KEY,
    YRS_OBJ_VAL,
    YRS_STOP,
    YRS_ERROR,
    YRS_SENTINEL
};

typedef struct {
    int state;
    int level;

    struct {
        uint32_t in_list       :1;
    } flags;

    fluid_object_t *current;
    fluid_object_t *previous;
    fluid_object_t *parent;
    fluid_object_t *root;
} config_reader_t;

#define debug_ptr(r) fprintf(stderr, "cur: %p par: %p\n", r->current, r->parent)

#define object_descend(r)  do {                   \
debug_ptr(r);\
        r->level++;                               \
        r->parent = r->current;                   \
        r->current = NULL;                        \
    } while (0);

#define object_ascend(r) do {                                               \
        debug_ptr(r);\
        if (r->level <= 0)                                                  \
            fexcept(FERROR_CONFIG_NESTING);                                 \
        r->current = r->current->parent;                                    \
        r->parent = r->current ? r->current->parent : NULL;                 \
        r->level--;                                                         \
    } while (0);

static ferror_t
config_process_yaml_event(config_reader_t *r, yaml_event_t *event)
{
    ferror_t e;

    switch (r->state) {
    case YRS_START:
        switch(event->type) {
        case YAML_STREAM_START_EVENT: break;
        case YAML_DOCUMENT_START_EVENT:
            r->state = YRS_OBJ_NEW;
            break;
        default: fexcept(FERROR_CONFIG_EVENT);
        }
        break;
    case YRS_OBJ_NEW:
        switch(event->type) {
        case YAML_MAPPING_START_EVENT:
            object_descend(r);
            e = fluid_object_new(NULL, &r->current);
            fexcept_proagate(e);
            fluid_object_cast_containier(r->current);
            r->state = YRS_OBJ_KEY;
            break;
        case YAML_DOCUMENT_END_EVENT:
            r->state = YRS_STOP;
            break;
        default: fexcept(FERROR_CONFIG_EVENT);
        }
        break;
    case YRS_OBJ_KEY:
        switch(event->type) {
        case YAML_SCALAR_EVENT:
            object_descend(r);
            e = fluid_object_new((char *)event->data.scalar.value, &r->current);
            fexcept_proagate(e);
            r->state = YRS_OBJ_VAL;
            break;
        case YAML_MAPPING_END_EVENT:
            object_ascend(r);
            r->state = YRS_OBJ_NEW;
            break;
        default:
            break;
        }
        break;
    case YRS_OBJ_VAL:
        switch(event->type) {
        case YAML_SCALAR_EVENT:
            e = fluid_object_add_value(r->current, (char *)event->data.scalar.value);
            fexcept_proagate(e);
            e = fluid_object_nest(r->parent, r->current);
            fexcept_proagate(e);
            object_ascend(r);
            r->state = YRS_OBJ_KEY;
            break;
        default:
            break;
        }
        break;
    case YRS_STOP:
        break;
    case YRS_ERROR:
        break;
    }

    return FERROR_OK;
}

ferror_t config_parse_yaml_buf(const char *input, size_t length)
{
    ferror_t e = FERROR_OK;
    yaml_event_t event;
    yaml_parser_t parser;
    config_reader_t r;

    memset(&r, 0, sizeof(config_reader_t));
    e = fluid_object_new("config", &r.root);
    fexcept_proagate(e);
    r.current = r.root;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (unsigned char *)input, length);

    while (r.state != YRS_STOP || r.state != YRS_ERROR) {

        if (!yaml_parser_parse(&parser, &event))
            fexcept_goto(FERROR_CONFIG_PARSER, error);

        e = config_process_yaml_event(&r, &event);
        fexcept_proagate_goto(e, error);

        yaml_event_delete(&event);
    }

error:
    yaml_parser_delete(&parser);
    return e;
}

ferror_t config_parse_yaml(const char *file)
{
    ferror_t e = FERROR_OK;
    char *buf = NULL;
    size_t size = 0;
    FILE *fd;

    if ((fd = fopen(file, "r")) == NULL)
        fexcept(FERROR_FILE_NOT_FOUND);

    if (file_read_all(fd, &buf, &size))
        fexcept_goto(FERROR_FILE_NOT_FOUND, error);

    e = config_parse_yaml_buf(buf, size);
    fexcept_proagate_goto(e, error);

error:
    safe_free(buf);
    fclose(fd);
    return e;
}
