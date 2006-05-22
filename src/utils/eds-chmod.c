/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Catalog - Change basic permissions
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: eds-chmod.c,v 1.1 2006-05-22 15:17:55 szamsu Exp $ 
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
const char *tool_usage = "MODE ITEM [ITEM...]";

/* Help message */
const char *tool_help = 
	"\t-b NUM\t\tNumber of updates in a batch\n"
	"Syntax of mode is: [uoga]*(+|-|=)[pdrwlxgs]+\n";

/* chmod-specific command line options */
const char *tool_options = "b:";

/* List of mode updates */
static GList *cmds;

/* Batching factor */
static unsigned batch_factor;

/* Batch operation queue */
static char **items;
static glite_catalog_Permission **perms;
static unsigned batchcount;


/**********************************************************************
 * Tool implementation
 */

static int do_batch(glite_catalog_ctx *ctx)
{
	unsigned i;
	int ret;

	if (!batchcount)
		return 0;

	ret = glite_metadata_setPermission_multi(ctx, batchcount,
		(const char **)items, (const glite_catalog_Permission **)perms);

	for (i = 0; i < batchcount; i++)
	{
		if (!ret)
			info("Changed permissions on %s", items[i]);
		g_free(items[i]);
		glite_catalog_Permission_free(ctx, perms[i]);
	}
	batchcount = 0;
	return ret;
}

static int do_chmod(glite_catalog_ctx *ctx, const char *item)
{
	int ret;

    glite_catalog_Permission *permission = glite_metadata_getPermission(ctx,item);

	if (!permission)
	{
		glite_catalog_set_error(ctx,
			GLITE_CATALOG_ERROR_UNKNOWN,
			"Missing permission for %s", item);
		return -1;
	}

	ret = apply_modes(permission, cmds);
	/* Call setPermission() only if there was a change */
	if (ret)
	{
		items[batchcount] = g_strdup(item);
		perms[batchcount] =
			glite_catalog_Permission_clone(ctx, permission);
		if (!items[batchcount] || !perms[batchcount])
		{
			error("eds-chmod: Out of memory");
			return -1;
		}
		batchcount++;

		if (batchcount >= batch_factor)
			return do_batch(ctx);
	}

	return 0;
}

/* Perform an chmod operation */
int tool_doit(glite_catalog_ctx *ctx, int argc, char *argv[])
{
	int i, ret;

	if (argc < 2)
	{
		error("eds-chmod: Wrong number of arguments");
		return EXIT_FAILURE;
	}

	/* Parse the mode argument */
	cmds = parse_modespec(argv[0]);
	if (!cmds)
	{
		error("eds-chmod: Illegal mode argument");
		return EXIT_FAILURE;
	}

	/* Set the default batch factor */
	if (!batch_factor)
		batch_factor = DEFAULT_BATCH_FACTOR;

	items = g_new(char *, batch_factor);
	perms = g_new(glite_catalog_Permission *, batch_factor);

	ret = 0;
	for (i = 1; i < argc; i++)
	{
        ret = do_chmod(ctx, argv[i]);
		if (ret)
		{
			error("eds-chmod: %s", glite_catalog_get_error(ctx));
			break;
		}
	}

	/* Process the entries still in the queue */
	if (!ret)
	{
		ret = do_batch(ctx);
		if (ret)
			error("eds-chmod: %s", glite_catalog_get_error(ctx));
	}

	if (ret)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/**********************************************************************
 * Command line parsing
 */

/* Parse chmod-specific command line options */
int tool_parse_cmdline(int c, char *opt_arg)
{
	char *p;

	switch (c)
	{
		case 'b':
			batch_factor = strtoul(opt_arg, &p, 10);
			if (p && *p)
			{
				error("Invalid batch factor");
				return -1;
			}
			return 0;
		default:
			break;
	}
	return -1;
}
