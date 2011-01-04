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

int clean_src (int argc, char **argv)
{
	FILE *find;
	char buf[512], *p;
	char pwd[256];
	
	if ((find = popen ("find " PACKAGE_DIR " -name \"build.sh\"", "r")) == NULL)
		return EXIT_FAILURE;

	while (fgets (buf, 511, find))
	{
		if ((p = strchr (buf, '\n'))) *p = 0;

		getcwd (pwd, 255);
		if (!(p = strrchr (buf, '/')))
			continue;

		*p = 0;
		chdir (buf);

		printf ("\033[32;01m===>\033[00m Cleaning '%s'\n", buf);
		if (EXIT_SUCCESS != execute ("sh build.sh clean"))
		{
			perror ("clean()");
			return EXIT_FAILURE;
		}

		chdir (pwd);
	}

	pclose (find);

	return EXIT_SUCCESS;
}

int clean_pkg (int argc, char **argv)
{
	if (EXIT_SUCCESS != execute ("find " PACKAGE_DIR " -name \"archive.tgz\" -print -delete"))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/* vim: set ts=4 shiftwidth=4: */
