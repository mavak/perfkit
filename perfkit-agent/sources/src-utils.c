/* src-utils.c
 *
 * Copyright (C) 2010 Andrew Stiegmann <andrew.stiegmann@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * XXX: We should move this to a common utils directory sometime in
 *      the near future.
 */

#include <fcntl.h>

#include "src-utils.h"

/**
 * src_utils_str_tok:
 * @delim: (in): The delimeter.
 * @ptr: (in): The string to tokenize.
 *
 * Tokenizes string inline by replacing the delimiting charcter with a
 * \0.
 *
 * Returns: Pointer to the beginning of the string after the token or
 *          %NULL if at the end of the string
 **/
gchar *
src_utils_str_tok (const gchar  delim,
                   gchar       *ptr)
{
	for (; *ptr; ptr++) {
		if (*ptr == delim) {
			*ptr = '\0';
			return ++ptr;
		}
	}
	return NULL;
}


/**
 * src_utils_read_file:
 * @path: (in): Path to file.
 * @buffer: (inout): The buffer to read into.  Can be NULL.
 * @bsize: (in): The size of the buffer if buffer is not NULL.
 *
 * Will read a file into a given buffer or, if buffer is NULL, allocate
 * a new buffer and read into it.  If you do not give this function a
 * buffer, you are responsible for freeing the buffer when you are done
 * using it.
 *
 * Returns: pointer to a buffer containg file data.
 **/
gchar *
src_utils_read_file (const gchar *path,
                     gchar       *buffer,
                     gssize       bsize)
{
	gint fd;
	ssize_t bytesRead;

	g_return_val_if_fail(path != NULL, NULL);

	if (buffer != NULL) {
		fd = open(path, O_RDONLY);

		if (fd < 0) {
			return NULL;
		}

		bytesRead = read(fd, buffer, bsize-1);
		close(fd);

		if (bytesRead < 0) {
			return NULL;
		}

		buffer[bytesRead] = '\0';
		return buffer;
	} else {
		gchar *contents;
		g_file_get_contents(path, &contents, NULL, NULL);
		return contents;
	}
}
