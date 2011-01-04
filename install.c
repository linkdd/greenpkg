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

int install (int argc, char **argv)
{
	char *pkg_name = NULL, *errmsg;
	char sql[256];
	char choose;
	int i, x;

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

		if (SQLITE_OK != sqlite3_exec (database, sql, sql_install_select, &tmp, &errmsg))
		{
			fprintf (stderr, "SQL Error: %s\n", errmsg);
			sqlite3_free (errmsg);
			sqlite3_close (database);
			return EXIT_FAILURE;
		}

		if (list_length (tmp) == 0)
		{
			fprintf (stderr, "No packages found for : %s\n", argv[i]);
			sqlite3_close (database);
			return EXIT_FAILURE;
		}
		/* Ask user to delete useless packages */
		else if (list_length (tmp) > 1)
		{
			printf ("Warning: There are several packages which contains '%s' :\n", argv[i]);

			for (x = 0; x < list_length (tmp); ++x)
			{
				struct list_t *local_tmp = list_get (tmp, x);
				printf ("(%d) - \033[34;01m%s \033[33;01m%s\033[00m\n", x, local_tmp->name, (local_tmp->state ? "(installed)" : ""));
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

	if (1 == get_conflicts ())
	{
		list_clear (&head);
		sqlite3_close (database);
		return EXIT_FAILURE;
	}

	/* ask the user one more time */
	i = list_length (head);
	printf ("%d package%s will be installed :\n", i, (i > 1 ? "s" : ""));

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
			if (process_to_install () == EXIT_FAILURE)
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

int local (int argc, char **argv)
{
	char command[256];
	char pwd[256];
	char *dest, *p;
	int i;

	for (i = 2; i < argc; ++i)
	{
		if (strchr (argv[i], '/') != NULL)
		{
			printf ("The package need to be in the current directory.\n");
			return EXIT_FAILURE;
		}

		printf ("\033[32;01m==>\033[00m Installing package \033[34;01m%s\033[00m (\033[33;01m%d\033[00m/\033[33;01m%d\033[00m)...\n", argv[i], i - 1, argc - 2);
		if (!file_exists (argv[i]))
		{
			printf ("\033[31;01mPackage '%s': No such file or directory.\n", argv[i]);
			return EXIT_FAILURE;
		}

		dest = malloc (sizeof (char) * (strlen (argv[i]) + 1));
		memset (dest, 0, strlen (argv[i]) + 1);
		strncpy (dest, argv[i], strlen (argv[i])); /* copy link */
		p = strrchr (dest, '.'); /* check '.tgz' */
		*p = 0; /* delete '.tgz' */

		/* Create directory */
		mkdir (dest, 0777);

		/* Extract package */
		printf ("\033[32;01m==>\033[00m Uncompress %s...\n", argv[i]);
		snprintf (command, 255, "tar xzf %s -C %s", argv[i], dest);
		if (EXIT_SUCCESS != execute (command))
		{
			perror ("tar()");
			return EXIT_FAILURE;
		}

		/* setup FAKEROOT */
		snprintf (command, 255, "/usr/pkg/local/%s", dest);
		setenv ("FAKEROOT", command, 1);

		/* Install package */
		getcwd (pwd, 255);
		chdir (dest);

		printf ("\033[32;01m===>\033[00m Preinstallation...\n");
		if (EXIT_SUCCESS != execute ("sh build.sh preinstall"))
		{
			perror ("install()");
			return EXIT_FAILURE;
		}

		printf ("\033[32;01m===>\033[00m Building...\n");
		if (EXIT_SUCCESS != execute ("sh build.sh compile"))
		{
			perror ("install()");
			return EXIT_FAILURE;
		}

		printf ("\033[32;01m===>\033[00m Installation...\n");
		if (EXIT_SUCCESS != execute ("sh build.sh install"))
		{
			perror ("install()");
			return EXIT_FAILURE;
		}

		printf ("\033[32;01m===>\033[00m Postinstallation...\n");
		if (EXIT_SUCCESS != execute ("sh build.sh postinstall"))
		{
			perror ("install()");
			return EXIT_FAILURE;
		}

		chdir (pwd);
		unsetenv ("FAKEROOT");

		printf ("- \033[34;01m%s\033[00m installed.\n", argv[i]);

		/* free memory */
		*p = '.'; free (dest);
	}

	printf ("To uninstall those packages, you just need to run their\n");
	printf ("build script into /usr/pkg/local/<package> :\n");
	printf ("# sh build.sh uninstall\n");

	return EXIT_SUCCESS;
}

/* vim: set ts=4 shiftwidth=4: */
