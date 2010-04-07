/*
 * Copyright (c) Members of the EGEE Collaboration. 2006-2010.
 * See http://www.eu-egee.org/partners/ for details on the copyright
 * holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  GLite Data Catalog - List ACL entries
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: eds-getacl.c,v 1.4 2010-04-07 11:11:06 jwhite Exp $ 
 *  Release: $Name: not supported by cvs2svn $
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/catalog/metadata/c/metadata-simple.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tool-main.h"

/**********************************************************************
 * Global variables
 */

/* Usage message */
const char *tool_usage = "ITEM [ITEM...]";

/* Help message */
const char *tool_help = "";

/* getacl-specific command line options */
const char *tool_options = "";

/* True if recursion is requested */

/* True if this is not the first entry */
static int notfirst;


/**********************************************************************
 * Tool implementation
 */

/* compare permissions */
static int cmp_perm(glite_catalog_Permission *a, glite_catalog_Permission *b)
{
	unsigned i;

	if (!a || !b)
		return -1;

	if (strcmp(a->userName, b->userName) ||
	    strcmp(a->groupName, b->groupName) ||
	    (a->userPerm != b->userPerm) ||
	    (a->groupPerm != b->groupPerm) ||
	    (a->otherPerm != b->otherPerm) ||
	    (a->acl_cnt != b->acl_cnt))
		return -1;
		
	/* should be in same order */
	for (i = 0; i < a->acl_cnt; i++)
	{
		glite_catalog_ACLEntry *acl_a = a->acl[i];
		glite_catalog_ACLEntry *acl_b = b->acl[i];

		if ((acl_a->principalPerm != acl_b->principalPerm) ||
		    strcmp(acl_a->principal, acl_b->principal))
			return -1;
	}

	return 0;
}


static int list_acls(glite_catalog_ctx_list *ctx_list, const char* item)
{
	int i;
	int ret = 0;
        glite_catalog_Permission *permission_list[ctx_list->count];

	for(i = 0; i < ctx_list->count; i++) 
		permission_list[i] = glite_metadata_getPermission(ctx_list->ctx[i], item);

	if (!permission_list[0])
	{
		error("Missing permission for %s\n", item);
		ret = -1;
		goto fail;
	}

	/* check that all permissions are identical */
	if (!verbose_flag)
	{
		for(i = 1; i < ctx_list->count; i++)
		{
			if (cmp_perm(permission_list[0], permission_list[i]))
			{
				error("Corrupted permissions for %s\n", item);
				ret = -1;
				goto fail;
			}
		}
	}

	/* Print an empty line between entries */
	if (notfirst)
		printf("\n");
	else
		notfirst = 1;

	if (verbose_flag) {
		for (i = 0; i < ctx_list->count; i++)
		{
			printf("# ENTRY: %s\n", item);
			printf("# ENDPOINT: %s\n", glite_catalog_get_endpoint(ctx_list->ctx[i]));
			if (permission_list[i])
				print_acls(permission_list[i]);
			else
				printf("# Not exits!!!\n");
		}
	} else {
		printf("# ENTRY: %s\n", item);
		print_acls(permission_list[0]);
	}

fail:
	for(i = 0; i < ctx_list->count; i++)
		if (permission_list[i])
			glite_catalog_Permission_free(NULL, permission_list[i]);

	return ret;
}

int tool_doit(glite_catalog_ctx_list *ctx_list, int argc, char *argv[])
{
	int i, ret;

	if (!argc)
	{
		error("eds-getacl: Missing ITEM");
		return EXIT_FAILURE;
	}

	ret = EXIT_SUCCESS;
	for (i = 0; i < argc; i++)
	{
		if(list_acls(ctx_list,argv[i]))
			ret = EXIT_FAILURE;
	}

	return ret;
}

/**********************************************************************
 * Command line parsing
 */

/* Parse getacl-specific command line options */
int tool_parse_cmdline(int c, char *opt_arg G_GNUC_UNUSED)
{
	switch (c)
	{
		default:
			return -1;
	}
	return 0;
}
