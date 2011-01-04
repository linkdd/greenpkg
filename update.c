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

static sqlite3 *new_db;

int fetch_db (int argc, char **argv)
{
	char command[256];
	
	if (getenv ("PKG_PATH") == NULL)
	{
		fprintf (stderr, "Variable $PKG_PATH empty...\n");
		return EXIT_FAILURE;
	}

	if (!file_exists (PACKAGE_LIST))
	{
		printf ("\033[32;01m==>\033[00m Fetch database...\n");
		execute ("install -d " PACKAGE_DIR);
		snprintf (command, 255, "wget %s/package.list -O %s/package.list", getenv ("PKG_PATH"), PACKAGE_DIR);
		execute (command);
		return EXIT_SUCCESS;
	}

	if (sqlite3_open (PACKAGE_LIST, &database))
	{
		fprintf (stderr, "Can't open database : %s\n", sqlite3_errmsg (database));
		return EXIT_FAILURE;
	}

	printf ("\033[32;01m==>\033[00m Reading database...\n");
	snprintf (command, 255, "wget %s/package.list -O %s/package.list.new", getenv ("PKG_PATH"), PACKAGE_DIR);
	if (EXIT_SUCCESS != execute (command))
	{
		perror ("update()");
		sqlite3_close (database);
		return EXIT_FAILURE;
	}

	if (sqlite3_open (PACKAGE_LIST ".new", &new_db))
	{
		fprintf (stderr, "Can't open new database : %s\n", sqlite3_errmsg (new_db));
		sqlite3_close (database);
		return EXIT_FAILURE;
	}

	printf ("\033[32;01m==>\033[00m Writing database...\n");
	sqlite3_exec (database, "SELECT * FROM packages WHERE is_installed='1'", sql_world, (void *) new_db, NULL);
	remove (PACKAGE_LIST);
	rename (PACKAGE_LIST ".new", PACKAGE_LIST);

	sqlite3_close (database);
	sqlite3_close (new_db);

	return EXIT_SUCCESS;
}

int update (int argc, char **argv)
{
	char sql[256];
	int i, ret = EXIT_SUCCESS;

	if (getenv ("PKG_PATH") == NULL)
	{
		fprintf (stderr, "Variable $PKG_PATH empty...\n");
		return EXIT_FAILURE;
	}

	/* Update database first */
	if (EXIT_FAILURE == fetch_db (0, NULL))
		return EXIT_FAILURE;

	if (sqlite3_open (PACKAGE_LIST, &database))
	{
		fprintf (stderr, "Can't open database : %s\n", sqlite3_errmsg (database));
		return EXIT_FAILURE;
	}

	/* Update packages */
	if (argc == 2)
	{
		printf ("\033[32;01m==>\033[00m Updating the system...\n");
		ret = process_to_update ();
	}
	else
	{
		printf ("\033[32;01m==>\033[00m Updating packages...\n");

		list_clear (&head);
		for (i = 2; i < argc; ++i)
		{
			struct list_t *tmp = NULL;

			snprintf (sql, 255, "SELECT * FROM packages WHERE package LIKE '%%%s%%' AND is_installed='1'", argv[i]);
			sqlite3_exec (database, sql, sql_update_select, &tmp, NULL);

			if (list_length (tmp) == 0)
			{
				fprintf (stderr, "No package found for '%s'\n", argv[i]);
				sqlite3_close (database);
				list_clear (&head);
				return EXIT_FAILURE;
			}
			else if (list_length (tmp) > 1)
			{
				char *pkg_name = NULL;
				int x;

				printf ("Warning: There are several packages which contain '%s' :\n", argv[i]);

				for (x = 0; x < list_length (tmp); ++x)
					printf ("(%d) - %s\n", x, list_get (tmp, x)->name);
				printf ("Choose one : ");
				if (1 != scanf ("%d", &x))
					x = 0;

				clean_buf ();

				printf ("Package (%d) choosed.\n", x);

				/* save current package name */
				pkg_name = malloc (list_get (tmp, x)->size + 1);
				memset (pkg_name, 0, list_get (tmp, x)->size + 1);
				strncpy (pkg_name, list_get (tmp, x)->name, list_get (tmp, x)->size);
				x = list_get (tmp, x)->state;

				/* clear list and reset wanted package name */
				list_clear (&tmp);
				head = list_prepend (head, pkg_name, strlen (pkg_name), x);
				free (pkg_name);
			}
			else
			{
				head = list_prepend (head, tmp->name, tmp->size, tmp->state);
			}
			
			get_depend (head->name);
		}

		ret = process_to_update ();
	}

	list_clear (&head);
	sqlite3_close (database);
	return ret;
}

/* vim: set ts=4 shiftwidth=4: */
