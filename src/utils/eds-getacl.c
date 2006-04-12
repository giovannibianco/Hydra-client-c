/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Catalog - List ACL entries
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: eds-getacl.c,v 1.1 2006-04-12 15:56:18 szamsu Exp $ 
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
const char *tool_help =
	"\t-r\t\tList ACLs recursively\n";

/* getacl-specific command line options */
const char *tool_options = "r";

/* True if recursion is requested */
/*static int rec_flag;*/

/* True if this is not the first entry */
static int notfirst;


/**********************************************************************
 * Tool implementation
 */

static int list_acls(glite_catalog_ctx *ctx G_GNUC_UNUSED, const char* item)
{
/* 	/\* Ignore the second call for recursive traversal *\/ */
/* 	if (direction == GLITE_CATALOG_EXP_POST) */
/* 		return 0; */

        glite_catalog_Permission *permission = glite_metadata_getPermission(ctx,item);

	if (!permission)
	{
		glite_catalog_set_error(ctx,
			GLITE_CATALOG_ERROR_UNKNOWN,
			"Missing permission for %s\n", item);
		return -1;
	}

	/* Print an empty line between entries */
	if (notfirst)
		printf("\n");
	else
		notfirst = 1;

	printf("# ENTRY: %s\n", item);
	print_acls(permission);

	return 0;
}

int tool_doit(glite_catalog_ctx *ctx, int argc, char *argv[])
{
  /*glite_catalog_exp_flag flags;*/
	int i, ret;

	if (!argc)
	{
		error("meta-getacl: Missing ITEM");
		return EXIT_FAILURE;
	}

/* 	flags = GLITE_CATALOG_EXP_WITHPERM; */
/* 	if (rec_flag) */
/* 		flags |= GLITE_CATALOG_EXP_RECURSIVE; */

	ret = 0;
	for (i = 0; i < argc; i++)
	{
		ret = list_acls(ctx,argv[i]);
		if (ret)
		{
			error("meta-getacl: %s", glite_catalog_get_error(ctx));
			break;
		}
	}

	if (ret)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/**********************************************************************
 * Command line parsing
 */

/* Parse getacl-specific command line options */
int tool_parse_cmdline(int c, char *opt_arg G_GNUC_UNUSED)
{
	switch (c)
	{
		case 'r':
		  /*rec_flag++;*/
			break;
		default:
			return -1;
	}
	return 0;
}
