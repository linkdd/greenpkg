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

#ifndef __LOCAL_H
#define __LOCAL_H

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <sqlite3.h>
#include <pthread.h>

#define clean_buf()		do {    \
	scanf ("%*[^\n]");          \
	getchar ();                 \
} while (/* CONSTCOND */ 0)

extern sqlite3 *database;

enum
{
	ID = 0,
	Category,
	Package,
	Version,
	Dependancies,
	Conflicts,
	Informations,
	User,
	Timestamp,
	IsInstalled,
	FieldNumber
};

/* util.c */
int execute (char *command);
int file_exists (const char *path);

int process_to_update (void);
int process_to_install (void);

int is_installed (char *package);
int package_exists (char *package);

void get_depend (char *pkg);
int get_conflicts (void);

/* Actions */
int install (int argc, char **argv);        /* Install packages */
int local (int argc, char **argv);          /* Install local packages */
int uninstall (int argc, char **argv);      /* Uninstall packages */
int update (int argc, char **argv);         /* Update packages/system */
int fetch_db (int argc, char **argv);       /* Download new database */
int search (int argc, char **argv);         /* Search packages, get informations about packages */
int clean_src (int argc, char **argv);      /* Remove left-over files created by compilations */
int clean_pkg (int argc, char **argv);      /* Remove archive.tgz */
int help (int argc, char **argv);           /* Show program's help */
int version (int argc, char **argv);        /* Show program's version */

struct cmd_t
{
	const char *opt;                        /* action to do */
	int min_args;                           /* minimum number of arguments expected */
	int (*func) (int argc, char **argv);    /* function to call */
	const char *desc;                       /* description */
};

/* List */

struct list_t
{
	char *name;         /* package name */
	size_t size;        /* length of the member 'name' */
	int state;          /* package state (installed or not ?) */
	int downloaded;     /* package is downloaded ? (used for installation process) */

	struct list_t *next;
	struct list_t *prev;
};

extern struct list_t *head;

struct list_t *list_prepend (struct list_t *list, char *name, size_t len, int state);
int list_length (struct list_t *list);
struct list_t *list_get (struct list_t *list, int idx);
int list_find (struct list_t *list, char *name);
void list_clear (struct list_t **list);
struct list_t *list_sort (struct list_t *list, int (*func) (char *, char *));

#define PACKAGE_DIR     "../var/pkg/"
#define PACKAGE_LIST    PACKAGE_DIR "package.list"

/* SQLite3 callbacks functions */

#define SQL_DECLARE(func)	\
	int sql_##func (void *data, int argc, char **argv, char **column)

SQL_DECLARE (install_select);
SQL_DECLARE (uninstall_select);
SQL_DECLARE (update_select);
SQL_DECLARE (is_installed);
SQL_DECLARE (exists);
SQL_DECLARE (depend);
SQL_DECLARE (conflicts);
SQL_DECLARE (world);
SQL_DECLARE (search);

#endif /* __LOCAL_H */

/* vim: set ts=4 shiftwidth=4: */
