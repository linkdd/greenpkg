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

/* Execute a command */
int execute (char *command)
{
	pid_t pid;
	int status;

	if (command == NULL)
		return 1;

	if ((pid = fork ()) < 0)
		return -1;

	if (pid == 0)
	{
		char *argv[4];

		argv[0] = "sh";
		argv[1] = "-c";
		argv[2] = command;
		argv[3] = NULL;

		execv ("/bin/sh", argv);
		exit (127);
	}

	while (1)
	{
		if (waitpid (pid, &status, 0) == -1)
		{
			if (errno != EINTR)
				return -1;
		}
		else
			return status;
	}
}

/* Check if a file exists */
int file_exists (const char *path)
{
	FILE *f = fopen (path, "r");
	
	if (f != NULL)
	{
		fclose (f);
		return 1;
	}

	return 0;
}
/* Compare two versions */
int strcmpver (char *pkg1, char *pkg2)
{
	char *ver1 = strrchr (pkg1, '/') + 1;
	char *ver2 = strrchr (pkg2, '/') + 1;
	int i;

	if (!strcmp (ver1, ver2))
		return 0;
	
	for (i = 0; ver1[i] != 0 && ver2[i] != 0; ++i)
	{
		if (ver1[i] < ver2[i])
			return 1;
		else
			return -1;
	}

	return 0;
}

/* Check if a package is installed */
int is_installed (char *package)
{
	int ret = 0;
	char sql[256];
	char *name, *p;

	name = strchr (package, '/') + 1;
	p = strchr (name, '/');
	if (p != NULL) *p = 0;
	snprintf (sql, 255, "SELECT is_installed FROM packages WHERE package = '%s' AND version='%s'", name, p + 1);
	if (p != NULL) *p = '/';

	sqlite3_exec (database, sql, sql_is_installed, &ret, NULL);
	return ret;
}

/* Check if a package exists */
int package_exists (char *package)
{
	int ret;
	char sql[256];

	snprintf (sql, 255, "SELECT COUNT(*) AS nb FROM packages WHERE package = '%s'", package);
	sqlite3_exec (database, sql, sql_exists, &ret, NULL);

	return (ret > 0 ? 1 : 0);
}

/* Get dependancies of a package */
void get_depend (char *pkg)
{
	char sql[256];
	char *name, *p;

	if (pkg == NULL) return;

	name = strchr (pkg, '/') + 1;
	p = strrchr (pkg, '/');

	*p = 0;
	snprintf (sql, 255, "SELECT dependancies FROM packages WHERE package = '%s' AND version='%s'", name, &p[1]);
	*p = '/';

	sqlite3_exec (database, sql, sql_depend, NULL, NULL);
}

/* Check if the package is in conflicts with another package */
int get_conflicts (void)
{
	int i, l = list_length (head), ret = 0;
	char *cat, *pkg, *ver;
	char sql[256];

	for (i = 0; i < l; ++i)
	{
		struct list_t *tmp = list_get (head, i);

		cat = tmp->name;
		pkg = strchr (cat, '/') + 1;
		ver = strrchr (cat, '/') + 1;
		*(pkg - 1) = *(ver - 1) = 0;

		snprintf (sql, 255, "SELECT * FROM packages WHERE category='%s' AND package='%s' AND version='%s'", cat, pkg, ver);
		sqlite3_exec (database, sql, sql_conflicts, &ret, NULL);

		*(pkg - 1) = *(ver - 1) = '/';
	}

	return ret;
}

static void *thread_process_to_download (void *data)
{
	int i, l = list_length (head);
	char command[256];

	for (i = 0; i < l; ++i)
	{
		struct list_t *tmp = list_get (head, i);

		snprintf (command, 255, "%s/%s/archive.tgz", PACKAGE_DIR, tmp->name);
		if (!file_exists (command))
		{
			snprintf (command, 255, "wget %s/%s.tgz -O %s/%s/archive.tgz -a %s/dllog", getenv ("PKG_PATH"), tmp->name, PACKAGE_DIR, tmp->name, PACKAGE_DIR);
			if (EXIT_SUCCESS != execute (command))
			{
				perror ("download()");
				list_clear (&head);
				sqlite3_close (database);
				exit (EXIT_FAILURE);
			}
			tmp->downloaded = 1;
		}
	}

	return NULL;
}

/* Install packages */
int process_to_install (void)
{
	int i, l = list_length (head);
	char command[256];
	char pwd[256];
	pthread_t thread_download;

	if (getenv ("PKG_PATH") == NULL)
	{
		fprintf (stderr, "Variable $PKG_PATH empty...\n");
		return EXIT_FAILURE;
	}

	printf ("To show the download's progress :\n");
	printf ("  tail -f %s/dllog\n", PACKAGE_DIR);
	
	if (pthread_create (&thread_download, NULL, thread_process_to_download, NULL))
		return EXIT_FAILURE;

	for (i = 0; i < l; ++i)
	{
		struct list_t *tmp = list_get (head, i);
		char *name, *p;
	
		printf ("\033[32;01m==>\033[00m Installing package \033[34;01m%s\033[00m (\033[33;01m%d\033[00m/\033[33;01m%d\033[00m)...\n", tmp->name, i + 1, l);
		printf ("\033]2;%s (%d/%d)\007", tmp->name, i + 1, l);
	
		/* Create direcyory */
		snprintf (command, 255, "install -d %s/%s", PACKAGE_DIR, tmp->name);
		if (EXIT_SUCCESS != execute (command))
		{
			perror ("install()");
			return EXIT_FAILURE;
		}
	
		/* Fetch package */
		printf ("\033[32;01m==>\033[00m Fetch archive...\n");
		snprintf (command, 255, "%s/%s/archive.tgz", PACKAGE_DIR, tmp->name);
		while (tmp->downloaded == 0)
			sleep (1);
	
		/* Extract package */
		printf ("\033[32;01m==>\033[00m Uncompress archive.tgz...\n");
		snprintf (command, 255, "tar xzf %s/%s/archive.tgz -C %s/%s/", PACKAGE_DIR, tmp->name, PACKAGE_DIR, tmp->name);
		if (EXIT_SUCCESS != execute (command))
		{
			perror ("tar()");
			return EXIT_FAILURE;
		}

		/* setup FAKEROOT */
		p = strchr (tmp->name, '/') + 1;
		snprintf (command, 255, "/usr/pkg/%s", p);
		setenv ("FAKEROOT", command, 1);

		/* Install package */
		getcwd (pwd, 255);
		snprintf (command, 255, "%s/%s/", PACKAGE_DIR, tmp->name);
		chdir (command);

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

		/* Modify the database */
		name = strchr (tmp->name, '/') + 1;
		p = strchr (name, '/'); *p = 0;

		printf ("\033[32;01m==>\033[00m Writing database...\n");

		snprintf (command, 255, "UPDATE packages SET is_installed='1', user='%s', timestamp='%ld' WHERE package='%s' AND version='%s'", getenv ("USER"), time(NULL), name, p + 1);
		sqlite3_exec (database, command, NULL, NULL, NULL);

		*p = '/';

		printf ("- \033[34;01m%s\033[00m installed.\n", tmp->name);
	}

	return EXIT_SUCCESS;
}

/* Update packages */
int process_to_update (void)
{
	struct list_t *local_head = NULL;
	char sql[256];
	char choose;
	int i;


	/* check new packages */
	for (i = 0; i < list_length (head); ++i)
	{
		char *name, *category, *package, *p;
		struct list_t *tmp = list_get (head, i);

		/* save package name */
		name = malloc (tmp->size + 1);
		memset (name, 0, tmp->size + 1);
		strncpy (name, tmp->name, tmp->size);

		category = name;
		p = strrchr (name, '/'); *p = 0;
		package = strchr (name, '/') + 1;
		*(package - 1) = 0;

		/* check all versions */
		tmp = NULL;
		snprintf (sql, 255, "SELECT * FROM packages WHERE is_installed='0' AND category='%s' AND package='%s'", category, package);
		sqlite3_exec (database, sql, sql_update_select, &tmp, NULL);

		/* sort to use the last version */
		tmp = list_sort (tmp, strcmpver);
		if (tmp != NULL)
		{
			while (tmp->prev) tmp = tmp->prev;
			if (!list_find (local_head, tmp->name))
				local_head = list_prepend (local_head, tmp->name, tmp->size, tmp->state);
			list_clear (&tmp);
		}

		free (name);
	}

	list_clear (&head);
	head = local_head;
	
	if (list_length (head) == 0)
	{
		printf ("System already up-to-date.\n");
		return EXIT_SUCCESS;
	}

	/* Ask user */
	printf ("Those packages will be updated :\n");
	for (i = 0; i < list_length (head); ++i)
		printf ("\t- %s\n", list_get (head, i)->name);

	do
	{
		printf ("Are you sure ? (y/n) : ");
		if (1 != scanf ("%c", &choose))
			choose = 'n';

		clean_buf ();

		if (choose == 'y')
		{
			return process_to_install ();
		}
		else
		{
			printf ("Cancelling update...\n");
		}
	} while (choose != 'n' && choose != 'y');

	return EXIT_SUCCESS;
}

/* vim: set ts=4 shiftwidth=4: */
