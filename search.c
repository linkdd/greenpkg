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

int search (int argc, char **argv)
{
	char sql[256];
	char *errmsg;
	int i, info;

	if (!strcmp (argv[1], "search"))
		info = 0;
	else if (!strcmp (argv[1], "info"))
		info = 1;

	if (sqlite3_open (PACKAGE_LIST, &database))
	{
		fprintf (stderr, "Can't open database : %s\n", sqlite3_errmsg (database));
		return EXIT_FAILURE;
	}

	for (i = 2; i < argc; ++i)
	{
		char *p1, *p2;

		/* check package */
		p1 = strchr (argv[i], '/');
		if (p1 == NULL) /* format : <package> */
		{
			snprintf (sql, 255, "SELECT * FROM packages WHERE package LIKE '%%%s%%'", argv[i]);
		}
		else
		{
			p2 = strrchr (argv[i], '/');

			if (p1 == p2) /* format : <category>/<package> */
			{
				*p2 = 0;
				snprintf (sql, 255, "SELECT * FROM packages WHERE category LIKE '%%%s%%' AND package LIKE '%%%s%%'", argv[i], p2 + 1);
				*p2 = '/';
			}
			else /* format : <category>/<package>/<version> */
			{
				*p1 = *p2 = 0;
				snprintf (sql, 255, "SELECT * FROM packages WHERE category LIKE '%%%s%%' AND package LIKE '%%%s%%' AND version LIKE '%%%s%%'", argv[i], p1 + 1, p2 + 1);
				*p1 = *p2 = '/';
			}
		}

		if (SQLITE_OK != sqlite3_exec (database, sql, sql_search, &info, &errmsg))
		{
			fprintf (stderr, "SQL Error: %s\n", errmsg);
			sqlite3_free (errmsg);
			sqlite3_close (database);
			return EXIT_FAILURE;
		}

	}

	sqlite3_close (database);
	return EXIT_SUCCESS;
}

/* vim: set ts=4 shiftwidth=4: */
