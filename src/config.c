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
} yaml_reader_t;

#define SET_CURRENT(r, c)  do {        \
        r->previous = r->current;      \
        r->current = c;                \
    } while (0);

#define SET_CURRENT_TO_PARENT(r) do {        \
        if (r->current->parent != NULL)      \
            r->current = r->current->parent; \
    } while (0);

static ferror_t config_process_event(yaml_reader_t *r, yaml_event_t *event)
{
    ferror_t e;

    switch (r->state) {
    case YRS_START:
        switch(event->type) {
        case YAML_MAPPING_START_EVENT:
            r->parent = r->current;
            e = fluid_object_new(NULL, &r->current);
            fexcept_proagate(e);
            r->state = YRS_OBJ_KEY;
            break;
        default:
            break;
        }
        break;
    case YRS_OBJ_KEY:
        switch(event->type) {
        case YAML_SCALAR_EVENT:
            e = fluid_object_new((char *)event->data.scalar.value, &r->current);
            fexcept_proagate(e);
            r->state = YRS_OBJ_VAL;
            break;
        case YAML_MAPPING_END_EVENT:
        case YAML_SEQUENCE_END_EVENT:
            SET_CURRENT_TO_PARENT(r);
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
            fluid_object_nest(r->parent, r->current);
            r->state = YRS_OBJ_KEY;
            break;
        case YAML_SEQUENCE_START_EVENT: /* value list start */
            fluid_object_cast_list(r->current);
            r->state = YRS_START;
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
    yaml_reader_t reader;
    yaml_event_t event;
    yaml_parser_t parser;

    reader.state = YRS_START;
    reader.level = 0;
    e = fluid_object_new("config", &reader.root);
    fexcept_proagate(e);
    reader.parent = reader.root;
    reader.previous = NULL;
    reader.current = NULL;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (unsigned char *)input, length);

    while (reader.state != YRS_STOP || reader.state != YRS_ERROR) {

        if (!yaml_parser_parse(&parser, &event))
            fexcept_goto_error(FERROR_CONFIG_PARSER);

        e = config_process_event(&reader, &event);
        fexcept_proagate_goto_error(e);

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

    return FERROR_OK;

    if ((fd = fopen(file, "r")) == NULL)
        fexcept(FERROR_FILE_NOT_FOUND);

    if (file_read_all(fd, &buf, &size))
        fexcept_goto_error(FERROR_FILE_NOT_FOUND);

    e = config_parse_yaml_buf(buf, size);
    fexcept_proagate_goto_error(e);

error:
    safe_free(buf);
    fclose(fd);
    exit(0);
    return e;
}
