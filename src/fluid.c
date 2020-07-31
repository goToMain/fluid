/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utils/file.h>

#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[])
{
	FILE *fd;
	char *data;
	size_t size;
	lexer_t lex;

	if (argc != 2) {
		printf("Error: usage: %s: filename.html", argv[0]);
		return -1;
	}

	fd = fopen(argv[1], "r");
	if (fd == NULL) {
		printf("Error: Failed to open %s\n", argv[1]);
		return -1;
	}

	if (file_read_all(fd, &data, &size) != 0) {
		printf("Failed to read contents of file %s\n", argv[1]);
		return -1;
	}

	lexer_init(&lex, data, size);
	if (lexer_lex(&lex) != 0) {
		return -1;
	}

	if (parser_check(&lex) != 0) {
		return -1;
	}

	lexer_free(&lex);
	free(data);
	return 0;
}
