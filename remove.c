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

struct list_t *get_depend_of (char *package)
{
	struct list_t *tmp = NULL;
	char sql[256];

	snprintf (sql, 255, "SELECT * FROM packages WHERE dependancies LIKE '%%%s%%' AND is_installed='1'", package);
	sqlite3_exec (database, sql, sql_uninstall_select, &tmp, NULL);

	return tmp;
}

int process_to_uninstall (void)
{
	int i, l = list_length (head);
	char command[256];
	char pwd[256];

	for (i = 0; i < l; ++i)
	{
		struct list_t *tmp = list_get (head, i);
		char *p, *name;

		printf ("\033[32;01m==>\033[00m Uninstalling package \033[34;01m%s\033[00m (\033[33;01m%d\033[00m/\033[33;01m%d\033[00m)...\n", tmp->name, i + 1, l);
		printf ("\033]2;%s (%d/%d)\007", tmp->name, i + 1, l);

		/* setup FAKEROOT */
		p = strchr (tmp->name, '/') + 1;
		snprintf (command, 255, "/usr/pkg/%s", p);
		setenv ("FAKEROOT", command, 1);

		/* Uninstall package */
		getcwd (pwd, 255);
		snprintf (command, 255, "%s/%s/", PACKAGE_DIR, tmp->name);
		chdir (command);

		printf ("\033[32;01m===>\033[00m Uninstallation...\n");
		if (EXIT_SUCCESS != execute ("sh build.sh uninst"))
		{
			perror ("uninstall()");
			return EXIT_FAILURE;
		}

		chdir (pwd);
		unsetenv ("FAKEROOT");

		/* Modify the database */
		name = strchr (tmp->name, '/') + 1;
		p = strchr (name, '/'); *p = 0;
			
		printf ("\033[32;01m==>\033[00m Writing database...\n");
		snprintf (command, 255, "UPDATE packages SET is_installed='0' WHERE package='%s' AND version='%s'", name, p + 1);
		sqlite3_exec (database, command, NULL, NULL, NULL);
		*p = '/';

		printf ("- \033[34;01m%s\033[00m uninstalled.\n", tmp->name);
	}

	return EXIT_SUCCESS;
}

int uninstall (int argc, char **argv)
{
	char *pkg_name = NULL, *errmsg;
	char sql[256];
	char choose;
	int i, x, errors = 0;

	if (sqlite3_open (PACKAGE_LIST, &database))
	{
		fprintf (stderr, "Can't open database : %s\n", sqlite3_errmsg (database));
		return EXIT_FAILURE;
	}

	for (i = 2; i < argc; ++i)
	{
		struct list_t *tmp = NULL;
		char *p1, *p2;

		/* check package */
		p1 = strchr (argv[i], '/');
		if (p1 == NULL) /* format : <package> */
		{
			snprintf (sql, 255, "SELECT * FROM packages WHERE package LIKE '%%%s%%' AND is_installed='1'", argv[i]);
		}
		else
		{
			p2 = strrchr (argv[i], '/');

			if (p1 == p2) /* format : <category>/<package> */
			{
				*p2 = 0;
				snprintf (sql, 255, "SELECT * FROM packages WHERE category LIKE '%%%s%%' AND package LIKE '%%%s%%' AND is_installed='1'", argv[i], p2 + 1);
				*p2 = '/';
			}
			else /* format : <category>/<package>/<version> */
			{
				*p1 = *p2 = 0;
				snprintf (sql, 255, "SELECT * FROM packages WHERE category LIKE '%%%s%%' AND package LIKE '%%%s%%' AND version LIKE '%%%s%%' AND is_installed='1'", argv[i], p1 + 1, p2 + 1);
				*p1 = *p2 = '/';
			}
		}

		if (SQLITE_OK != sqlite3_exec (database, sql, sql_uninstall_select, &tmp, &errmsg))
		{
			fprintf (stderr, "SQL Error: %s\n", errmsg);
			sqlite3_free (errmsg);
			sqlite3_close (database);
			return EXIT_FAILURE;
		}

		if (list_length (tmp) == 0)
		{
			fprintf (stderr, "No installed packages found for : %s\n", argv[i]);
			sqlite3_close (database);
			return EXIT_FAILURE;
		}
		/* Ask user to delete useless packages */
		else if (list_length (tmp) > 1)
		{
			printf ("Warning: There are several packages which contains '%s'\n", argv[i]);

			for (x = 0; x < list_length (tmp); ++x)
			{
				printf ("(%d) - %s\n", x, list_get (tmp, x)->name);
			}
			printf ("Choose one : ");
			if (1 != scanf ("%d", &x))
				x = 0;

			clean_buf ();

			printf ("Package (%d) choosed.\n", x);

			/* save current package name */
			pkg_name = malloc (list_get (tmp, x)->size + 1);
			memset (pkg_name, 0, list_get (tmp, x)->size + 1);
			strncpy (pkg_name, list_get (tmp, x)->name, list_get (tmp, x)->size);

			/* clear list and reset wanted package name */
			list_clear (&tmp);
			head = list_prepend (head, pkg_name, strlen (pkg_name), 1);
			free (pkg_name);
		}
		else
		{
			head = list_prepend (head, tmp->name, tmp->size, tmp->state);
		}

		/* If an installed package depend of head->name, ask the user */
		if ((tmp = get_depend_of (head->name)) != NULL)
		{
			printf ("Those packages depend of '%s' and should not work after uninstallation :\n", head->name);
			for (x = 0; x < list_length (tmp); ++x)
				printf ("\t- %s\n", list_get (tmp, x)->name);

			do
			{
				printf ("Are you sure ? (y/n) : ");
				if (1 != scanf ("%c", &choose))
					choose = 'n';

				clean_buf ();

				if (choose == 'y')
					errors++;
				else if (choose == 'n')
				{
					printf ("Cancel installation...\n");
					list_clear (&head);
					sqlite3_close (database);
					return EXIT_SUCCESS;
				}
			} while (choose != 'n' && choose != 'y');
		}
	}

	/* ask user one more time */
	i = list_length (head);
	printf ("%d package%s will be uninstalled, with %d error%s :\n", i, (i > 1 ? "s" : ""), errors, (errors > 1 ? "s" : ""));

	for (x = 0; x < i; ++x)
	   printf ("\t- %s\n", list_get (head, x)->name);	

	do
	{
		printf ("Are you sure ? (y/n) : ");
		if (1 != scanf ("%c", &choose))
			choose = 'n';
		
		clean_buf ();

		if (choose == 'y')
		{
			if (process_to_uninstall () == EXIT_FAILURE)
			{
				list_clear (&head);
				sqlite3_close (database);
				return EXIT_FAILURE;
			}
		}
		else if (choose == 'n')
		{
			printf ("Cancel installation...\n");
		}
	} while (choose != 'n' && choose != 'y');

	list_clear (&head);
	sqlite3_close (database);

	return EXIT_SUCCESS;
}

/* vim: set ts=4 shiftwidth=4: */
