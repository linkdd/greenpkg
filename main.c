/*
 * 	    Copyright (C) 2009, David Delassus <linkdd@ydb.me>
 *
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the  nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "local.h"

static struct cmd_t entries[] =
{
	{ "install",    1,  install,    "Install packages" },
	{ "local",      1,  local,      "Install local packages" },
	{ "remove",     1,  uninstall,  "Uninstall packages" },
	{ "update",     0,  update,     "Update packages/system" },
	{ "fetchdb",    0,  fetch_db,   "Download database" },
	{ "search",     1,  search,     "Search packages in the repository" },
	{ "info",       1,  search,     "Get informations about packages" },
	{ "rm-src",     0,  clean_src,  "Remove left-over files created by compilations" },
	{ "rm-pkg",     0,  clean_pkg,  "Remove downloaded archives ('archive.tgz')" },
	{ "version",    0,  version,    "Print version informations" },
	{ "help",       0,  help,       "Print this help" },
	{ NULL, 0, NULL, NULL }
};

sqlite3 *database;

int help (int argc, char **argv)
{	
	int i;

	printf ("Usage: greenpkg options [packages...]\n\n");

	printf ("Options:\n");
	for (i = 0; entries[i].opt != NULL; ++i)
	{
		printf ("\t%s\t[%d] %s\n",
				entries[i].opt,
				entries[i].min_args,
				entries[i].desc);
	}

	printf ("\n[X] = Minimum number of arguments expected\n\n");
	return EXIT_SUCCESS;
}

int version (int argc, char **argv)
{
	printf ("GreenPKG v0.1, developped by David Delassus <linkdd@ydb.me>\n");
	printf ("Under the terms of the BSD License.\n");
	return EXIT_SUCCESS;
}

int main (int argc, char **argv)
{
	int i;

	if (argc < 2)
	{
		help (argc, argv);
		return EXIT_FAILURE;
	}

	for (i = 0; entries[i].opt != NULL; ++i)
	{
		if (!strcmp (entries[i].opt, argv[1]))
		{
			if (argc - 2 >= entries[i].min_args)
			{
				if (entries[i].func)
				{
					return entries[i].func (argc, argv);
				}

				fprintf (stderr, "%s: Not yet implemented\n", argv[1]);
				return EXIT_SUCCESS;
			}
			else
			{
				fprintf (stderr, "%s: Too few arguments\n", argv[1]);
				return EXIT_FAILURE;
			}
		}
	}

	fprintf (stderr, "Unknow option : %s, type 'help'\n", argv[1]);
	return EXIT_FAILURE;
}

/* vim: set ts=4 shiftwidth=4: */
