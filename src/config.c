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

#include "fobjects.h"
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

    fobject_t *root;
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
#if 1
    printf("YRS: %s Event: %s\n",
           yrs_state_name(r->state),
           yaml_event_name(event->type));
#endif

    return FERROR_OK;
}

ferror_t config_parse_yaml_buf(const char *input, size_t length)
{
    ferror_t e = FERROR_OK;
    yaml_event_t event;
    yaml_parser_t parser;
    config_reader_t r;

    reader.state = YRS_START;
    reader.level = 0;
    reader.root = fdict_new();

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
