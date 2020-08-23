/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stddef.h>

#include "ferrors.h"

ferror_t config_parse_yaml(const char *file);
ferror_t config_parse_yaml_buf(const char *input, size_t length);

#endif  /* _CONFIG_H_ */
