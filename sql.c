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

SQL_DECLARE (install_select)
{
	struct list_t *tmp = *(struct list_t **) data;
	char package[256];

	snprintf (package, 255, "%s/%s/%s", argv[Category], argv[Package], argv[Version]);
	tmp = list_prepend (tmp, package, strlen (package), (int) strtol (argv[IsInstalled], NULL, 2));
	*(struct list_t **) data = tmp;

	return 0;
}

SQL_DECLARE (uninstall_select)
{
	struct list_t *tmp = *(struct list_t **) data;
	char package[256];
	int installed = (int) strtol (argv[IsInstalled], NULL, 2);

	if (installed)
	{
		snprintf (package, 255, "%s/%s/%s", argv[Category], argv[Package], argv[Version]);
		tmp = list_prepend (tmp, package, strlen (package), installed);
		* (struct list_t **) data = tmp;
	}

	return 0;
}

SQL_DECLARE (update_select)
{
	struct list_t *tmp = *(struct list_t **) data;
	char name[256];

	snprintf (name, 255, "%s/%s/%s", argv[Category], argv[Package], argv[Version]);
	tmp = list_prepend (tmp, name, strlen (name), (int) strtol (argv[IsInstalled], NULL, 2));

	*(struct list_t **) data = tmp;
	return 0;
}

SQL_DECLARE (is_installed)
{
	*(int *) data = (int) strtol (argv[0], NULL, 2);
	return 0;
}

SQL_DECLARE (exists)
{
	*(int *) data = (int) strtol (argv[0], NULL, 10);
	return 0;
}

SQL_DECLARE (depend)
{
	char *p;

	if (argv[0] == NULL || strlen (argv[0]) == 0)
			return 0;

	p = strtok (argv[0], "\n");
	while (p != NULL)
	{
		char *tmp0, *tmp1;

		tmp0 = strchr (p, '/');
		tmp1 = strrchr (p, '/');
		*tmp0 = *tmp1 = 0;

		if (!package_exists (tmp0 + 1))
		{
			*tmp0 = *tmp1 = '/';
			fprintf (stderr, "\033[31;01mCan't resolve dependancies, the package '\033[34;01m%s\033[31;01m' doesn't exist.\033[00m\n", p);
			sqlite3_close (database);
			list_clear (&head);
			exit (EXIT_FAILURE);
		}

		*tmp0 = *tmp1 = '/';

		if (!list_find (head, p) && !is_installed (p))
			head = list_prepend (head, p, strlen (p), 0);

		get_depend (p);
		p = strtok (NULL, "\n");
	}

	return 0;
}

SQL_DECLARE (conflicts)
{
	char *p = strtok (argv[Conflicts], "\n");

	while (p)
	{
		if ((int) strtol (argv[IsInstalled], NULL, 2))
		{
			fprintf (stderr, "- \033[31;01m%s/%s/%s blocked by %s\033[00m\n", argv[Category], argv[Package], argv[Version], p);
			*(int *) data = 1;
		}
		p = strtok (NULL, "\n");
	}

	return 0;
}

SQL_DECLARE (world)
{
	sqlite3 *new_db = (sqlite3 *) data;
	char str[256];

	snprintf (str, 255, "UPDATE packages SET is_installed='1' WHERE category='%s' AND package='%s' AND version='%s'", argv[Category], argv[Package], argv[Version]);
	sqlite3_exec (new_db, str, NULL, NULL, NULL);
	
	snprintf (str, 255, "%s/%s/%s", argv[Category], argv[Package], argv[Version]);
	head = list_prepend (head, str, strlen (str), 1);

	return 0;	
}

SQL_DECLARE (search)
{
	int installed = (int) strtol (argv[IsInstalled], NULL, 2);
	int info = *(int *) data;

	printf ("- \033[34;01m%s/%s/%s\033[00m \033[33;01m%s\033[00m\n",
			argv[Category], argv[Package], argv[Version],
			(installed ? "(installed)" : ""));

	if (info)
	{
		printf ("%s\n", argv[Informations]);

		if (argv[User] != NULL && strlen (argv[User]) > 0)
		{
			time_t t = (time_t) strtol (argv[Timestamp], NULL, 10);

			if (installed)
				printf ("Installed by \033[31;01m%s\033[00m, %s", argv[User], ctime (&t));
			else
				printf ("Uninstalled by \033[31;01m%s\033[00m, %s", argv[User], ctime (&t));
		}

		printf ("\n");
	}

	return 0;
}

/* vim: set ts=4 shiftwidth=4: */
