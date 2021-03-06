// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/file.c
 *
 * File handling functions
 *
 * Copyright (C) 2018-2020 SCANOSS.COM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

bool is_file(char *path)
{
	struct stat pstat;
	if (!stat(path, &pstat))
		if (S_ISREG(pstat.st_mode))
			return true;
	return false;
}

bool is_dir(char *path)
{
	struct stat pstat;
	if (!stat(path, &pstat))
		if (S_ISDIR(pstat.st_mode))
			return true;
	return false;
}

uint64_t get_file_size(char *path)
{
	uint64_t length = 0;
	FILE *file = fopen(path, "rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fclose(file);
	}
	return length;
}

void file_md5(char *filepath, uint8_t *md5_result)
{

	/* Read file contents into buffer */
	FILE *in = fopen(filepath, "rb");
	fseek(in, 0L, SEEK_END);
	long filesize = ftell(in);

	if (!filesize)
	{
		MD5(NULL, 0, md5_result);
	}

	else
	{
		/* Read file contents */
		fseek(in, 0L, SEEK_SET);
		uint8_t *buffer = malloc(filesize);
		if (!fread(buffer, filesize, 1, in)) fprintf(stderr, "Warning: cannot open file %s\n", filepath);

		/* Calculate MD5sum */
		MD5(buffer, filesize, md5_result);
		free (buffer);
	}

	fclose(in);
}

void read_file(char *out, char *path, uint64_t maxlen)
{

	char *src;
	uint64_t length = 0;
	out[0] = 0;

	if (!is_file(path))
	{
		return;
	}

	FILE *file = fopen(path, "rb");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);
		src = calloc(length, 1);
		if (src)
		{
			fread(src, 1, length, file);
		}
		fclose(file);
		if (maxlen > 0)
			if (length > maxlen)
				length = maxlen;
		memcpy(out, src, length);
		free(src);
	}
}

