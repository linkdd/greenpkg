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

struct list_t *head = NULL;

struct list_t *list_prepend (struct list_t *list, char *name, size_t len, int state)
{
	struct list_t *tmp = malloc (sizeof (struct list_t));

	if (tmp != NULL)
	{
		tmp->downloaded = 0;
		tmp->state = state;
		tmp->size = len;
		tmp->name = malloc (len + 1);
		memset (tmp->name, 0, len + 1);
		strncpy (tmp->name, name, len);
		
		tmp->next = list;
		tmp->prev = NULL;
		if (list != NULL)
			list->prev = tmp;
		list = tmp;
	}

	return list;
}

int list_length (struct list_t *list)
{
	struct list_t *tmp = list;
	int i;

	for (i = 0; tmp != NULL; ++i)
		tmp = tmp->next;

	return i;
}

struct list_t *list_get (struct list_t *list, int idx)
{
	struct list_t *tmp = list;
	int i;

	for (i = 0; i < idx; ++i)
	{
		if (tmp)
			tmp = tmp->next;
		else
			return NULL;
	}

	return tmp;
}

int list_find (struct list_t *list, char *name)
{
	struct list_t *tmp = list;

	while (tmp)
	{
		if (!strcmp (tmp->name, name))
			return 1;

		tmp = tmp->next;
	}

	return 0;
}

void list_clear (struct list_t **list)
{
	struct list_t *tmp;
	struct list_t *temp;

	if (list == NULL) return;
	tmp = *list;

	while (tmp)
	{
		free (tmp->name);
		temp = tmp->next;
		free (tmp);
		tmp = temp;
	}
	
	*list = NULL;
}

struct list_t *list_sort (struct list_t *list, int (*func) (char *, char *))
{
	struct list_t *tmp = list;
	int sorted = 0;

	if (tmp == NULL) return list;

	while (tmp->next)
	{
		if (1 == func (tmp->name, tmp->next->name))
		{
			char *tmp_name;
			int tmp_state;
			size_t tmp_size;

			tmp_name = tmp->next->name;
			tmp_size = tmp->next->size;
			tmp_state = tmp->next->state;

			tmp->next->name = tmp->name;
			tmp->next->size = tmp->size;
			tmp->next->state = tmp->state;

			tmp->name = tmp_name;
			tmp->size = tmp_size;
			tmp->state = tmp_state;

			sorted = 1;
		}

		tmp = tmp->next;
	}

	if (sorted)
		tmp = list_sort (tmp, func);

	return tmp;
}

/* vim: set ts=4 shiftwidth=4: */
