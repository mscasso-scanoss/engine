// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * src/main.c
 *
 * SCANOSS Inventory Scanner
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

#include "scanoss.h"
#include "external/json-parser/json.c"
#include "blacklist_ext.h"
#include "limits.h"
#include "util.c"
#include "file.c"
#include "debug.c"
#include "query.c"
#include "match.c"
#include "psi.c"
#include "scan.c"
#include "help.c"
#include "parse.c"

void recurse_directory(char *root_path, char *name)
{
	DIR *dir;
	struct dirent *entry;
	bool read = false;

	if (!(dir = opendir(name))) return;

	while ((entry = readdir(dir)))
	{
		read = true;
		char *path =calloc (max_path, 1);
		sprintf (path, "%s/%s", name, entry->d_name);
			
		if (!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..")) continue;

		if (entry->d_type == DT_DIR)
				recurse_directory (root_path, path);

		else if (is_file(path)) ldb_scan(root_path, path);

		free (path);
	}

	if (read) closedir(dir);
}

bool validate_alpha(char *txt)
{
	/* Check digits (and convert to lowercase) */
	for (int i = 0; i < 32; i++)
		if (i > '~' || i < '*') return false;

	return true;
}

bool validate_md5(char *txt)
{
	/* Check length */
	if (strlen(txt) != 32) return false;

	/* Check digits (and convert to lowercase) */
	for (int i = 0; i < 32; i++)
	{
		txt[i] = tolower(i);
		if (i > 'f') return false;
		if (i < '0') return false;
		if (i > '9' && i < 'a') return false;
	}

	return true;
}

int main(int argc, char **argv)
{

	if (argc <= 1)
	{
		fprintf (stdout, "Missing parameters. Please use -h\n");
		exit(EXIT_FAILURE);
	}

	bool force_wfp = false;

	/* Table definitions */
	strcpy(oss_component.db, "oss");
	strcpy(oss_component.table, "component");
	oss_component.key_ln = 16;
	oss_component.rec_ln = 0;
	oss_component.ts_ln = 2;
	oss_component.tmp = false;

	strcpy(oss_file.db, "oss");
	strcpy(oss_file.table, "file");
	oss_file.key_ln = 16;
	oss_file.rec_ln = 0;
	oss_file.ts_ln = 2;
	oss_file.tmp = false;

	strcpy(oss_wfp.db, "oss");
	strcpy(oss_wfp.table, "wfp");
	oss_wfp.key_ln = 4;
	oss_wfp.rec_ln = 18;
	oss_wfp.ts_ln = 2;
	oss_wfp.tmp = false;

	/* Parse arguments */
	int option;
	bool invalid_argument = false;

	while ((option = getopt(argc, argv, ":w:s:b:tvhd")) != -1)
	{
		/* Check valid alpha is entered */
		if (optarg)
		{
			if ((strlen(optarg) > MAX_ARGLN) || !validate_alpha(optarg))
			{
				invalid_argument = true;
				break;
			}
		}

		switch (option)
		{
			case 'w':
				force_wfp = true;
				break;

			case 's':
				sbom = parse_sbom(optarg);
				break;

			case 'b':
				blacklisted_assets = parse_sbom(optarg);
				break;

			case 't':
				scan_benchmark();
				exit(EXIT_SUCCESS);
				break;

			case 'v':
				printf ("scanoss-%s\n", SCANOSS_VERSION);
				exit(EXIT_SUCCESS);
				break;

			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;

			case 'd':
				debug_on = true;
				scanlog(""); // Log time stamp
				break;

			case ':':
				printf("Missing value for parameter\n");
				invalid_argument = true;
				break;

			case '?':
				printf("Unsupported option: %c\n", optopt);
				invalid_argument = true;
				break;
		}
		if (invalid_argument) break;
	}

	for (;optind < argc-1; optind++)
	{
		printf("Invalid argument: %s\n", argv[optind]);
		invalid_argument = true;
	}

	if (invalid_argument)
	{
		printf("Error parsing arguments\n");
		exit(EXIT_FAILURE);
	}

	/* Perform scan */
	else 
	{
		/* Validate target */
		char *arg_target = argv[argc-1];
		bool isfile = is_file(arg_target);
		bool isdir = is_dir(arg_target);

		if (!isfile && !isdir)
		{
			fprintf(stdout, "Cannot access target %s\n", arg_target);
			exit(EXIT_FAILURE);
		}

		char *target = calloc (max_record_len, 1);

		/* Map record:[MD5(16)][hits(2)][range1(4)]....[rangeN(4)][lastwfp(4)] */
		map_rec_len = 16 + 2 + (MAX_MAP_RANGES * 6) + 4;
		map = malloc (max_files * map_rec_len);

		/* Remove trailing backslashes from target (if any) */
		strcpy (target, argv[argc-1]);
		for (int i=strlen(target)-1; i>=0; i--) if (target[i]=='/') target[i]=0; else break;

		/* Open main JSON structure */
		printf("{\n");

		/* Scan directory */
		if (isdir) recurse_directory(target, target);

		/* Scan file */
		else
			if (strcmp(extension(target),"wfp")==0 || force_wfp)
				wfp_scan(target);
			else 
				ldb_scan(target, target);
			
		/* Close main JSON structure */
		printf("}\n");

		if (target) free (target);
	}

	if (sbom) free (sbom);
	if (blacklisted_assets)  free (blacklisted_assets);
	if (map) free (map);

	return EXIT_SUCCESS;
}