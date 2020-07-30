/*
 * Copyright (c) 2019 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utils/file.h>

#include "common.h"

int main(int argc, char *argv[])
{
	FILE *fd;
	char *data;
	size_t size;
	lexer_t lex;

	if (argc != 2) {
		printf("Error: usage: %s: filename.html", argv[0]);
		exit(-1);
	}

	fd = fopen(argv[1], "r");
	if (fd == NULL) {
		printf("Error: Failed to open %s\n", argv[1]);
		exit(-1);
	}

	if (file_read_all(fd, &data, &size) != 0) {
		printf("Failed to read contents of file %s\n", argv[1]);
		exit(-1);
	}

	lexer_init(&lex);

	if (lexer_lex(&lex, data, size) != 0) {
		printf("Lexer failed\n");
		exit(-1);
	}

	lexer_free(&lex);
	free(data);

	return 0;
}
