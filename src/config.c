/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <yaml.h>
#include <utils/file.h>
#include <utils/stack.h>

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
    YRS_STOP
};

typedef struct {
    int state;
    int level;

    struct {
        uint32_t in_list       :1;
    } flags;

    stack_t stack;
    fluid_object_t *current;
} config_reader_t;

const char *yrs_state_name(enum yaml_reader_state_e state)
{
    switch (state) {
    case YRS_START:      return "START";
    case YRS_OBJ_NEW:    return "OBJ_NEW";
    case YRS_OBJ_KEY:    return "OBJ_KEY";
    case YRS_OBJ_VAL:    return "OBJ_VAL";
    case YRS_STOP:       return "STOP";
    }
    return "";
}

const char *yaml_event_name(yaml_event_type_t event)
{
    switch (event) {
    case YAML_NO_EVENT:               return "NO_EVENT";
    case YAML_STREAM_START_EVENT:     return "STREAM_START_EVENT";
    case YAML_STREAM_END_EVENT:       return "STREAM_END_EVENT";
    case YAML_DOCUMENT_START_EVENT:   return "DOCUMENT_START_EVENT";
    case YAML_DOCUMENT_END_EVENT:     return "DOCUMENT_END_EVENT";
    case YAML_ALIAS_EVENT:            return "ALIAS_EVENT";
    case YAML_SCALAR_EVENT:           return "SCALAR_EVENT";
    case YAML_SEQUENCE_START_EVENT:   return "SEQUENCE_START_EVENT";
    case YAML_SEQUENCE_END_EVENT:     return "SEQUENCE_END_EVENT";
    case YAML_MAPPING_START_EVENT:    return "MAPPING_START_EVENT";
    case YAML_MAPPING_END_EVENT:      return "MAPPING_END_EVENT";
    }
    return "";
}

#define EVENT_VAL(e) (char *)e->data.scalar.value

// #define object_descend(r)  do {                   \
//         r->current->parent = r->current;          \
//         r->parent = r->current;                   \
//         r->current = NULL;                        \
//         r->level++;                               \
//     } while (0);

// #define object_ascend(r) do {                                               \
//         if (r->level <= 0)                                                  \
//             fexcept(FERROR_CONFIG_NESTING);                                 \
//         r->current = r->current->parent;                                    \
//         r->parent = r->current->parent;                                     \
//         r->level--;                                                         \
//     } while (0);

void object_descend(config_reader_t *r)
{
    stack_push(&r->stack, &r->current->node);
}

ferror_t object_ascend(config_reader_t *r)
{
    stack_node_t node;
    fluid_object_t *obj;

    if (stack_pop(&r->stack, &node))
        fexcept(FERROR_CONFIG_NESTING);
    obj = CONTAINER_OF(&node, fluid_object_t, node);
}

static ferror_t
config_process_yaml_event(config_reader_t *r, yaml_event_t *event)
{
#if 1
    printf("YRS: %s Event: %s\n",
           yrs_state_name(r->state),
           yaml_event_name(event->type));
#endif

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
            FEX( fluid_object_cast_containier(r->current) );
            r->state = YRS_OBJ_KEY;
            break;
        case YAML_DOCUMENT_END_EVENT:
        case YAML_STREAM_END_EVENT:
            r->state = YRS_STOP;
            break;
        case YAML_MAPPING_END_EVENT:
            FEX( fluid_object_nest(r->parent ? r->parent : r->root, r->current) );
            object_ascend(r);
            break;
        case YAML_SCALAR_EVENT:
            object_descend(r);
            stack_push(&r->stack, &r->current->node);
            FEX( fluid_object_new(EVENT_VAL(event), r->parent, &r->current) );
            r->state = YRS_OBJ_VAL;
            break;
        default: fexcept(FERROR_CONFIG_EVENT);
        }
        break;
    case YRS_OBJ_KEY:
        switch(event->type) {
        case YAML_SCALAR_EVENT:
            object_descend(r);
            stack_push(&r->stack, &r->current->node);
            printf("New Key: %s\n", EVENT_VAL(event));
            FEX( fluid_object_new(EVENT_VAL(event), r->parent, &r->current) );
            r->state = YRS_OBJ_VAL;
            break;
        case YAML_MAPPING_END_EVENT:
            FEX( fluid_object_nest(r->parent ? r->parent : r->root, r->current) );
            object_ascend(r);
            r->state = YRS_OBJ_NEW;
            break;
        default: fexcept(FERROR_CONFIG_EVENT);
        }
        break;
    case YRS_OBJ_VAL:
        switch(event->type) {
        case YAML_SCALAR_EVENT:
            printf("New Val: %s\n", EVENT_VAL(event));
            FEX( fluid_object_add_value(r->current, EVENT_VAL(event)) );
            FEX( fluid_object_nest(r->parent ? r->parent : r->root, r->current) );
            object_ascend(r);
            r->state = YRS_OBJ_KEY;
            break;
        case YAML_MAPPING_START_EVENT:
            FEX( fluid_object_cast_containier(r->current) );
            r->state = YRS_OBJ_KEY;
            break;
        default: fexcept(FERROR_CONFIG_EVENT);
        }
        break;
    case YRS_STOP:
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
    stack_init(&r.stack);

    FEX( fluid_object_new("config", NULL, &r.current) );
    stack_push(&r.stack, &r.current->node);

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (unsigned char *)input, length);

    while (r.state != YRS_STOP) {

        if (!yaml_parser_parse(&parser, &event))
            fexcept_goto(FERROR_CONFIG_PARSER, error);

        e = config_process_yaml_event(&r, &event);
        fexcept_proagate_goto(e, error);

        yaml_event_delete(&event);
    }

    fluid_object_dump(r.root);

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
