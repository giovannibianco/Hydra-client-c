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
 *  GLite Data Catalog - Change basic permissions
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: eds-chmod.c,v 1.3 2010-04-07 11:11:06 jwhite Exp $ 
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

static int do_batch(glite_catalog_ctx_list *ctx_list)
{
	int ret = 0;
	int i;

	if (!batchcount)
		return 0;

	for(i = 0; i < ctx_list->count; i++)
	{
		ret |= glite_metadata_setPermission_multi(ctx_list->ctx[i], batchcount,
			(const char **)items, (const glite_catalog_Permission **)perms);
	}

	for (i = 0; i < (int)batchcount; i++)
	{
		if (ret)
			error("Change failed on %s", items[i]);
		else
			info("Changed permissions on %s", items[i]);
		g_free(items[i]);
		glite_catalog_Permission_free(NULL, perms[i]);
	}
	batchcount = 0;

	return ret;
}

/* This cache requests, call item=NULL at the end to flush */
static int do_chmod(glite_catalog_ctx_list *ctx_list, const char *item)
{
	glite_catalog_Permission *permission;
	int ret = 0;

	if (item == NULL) /* flush the queue */
		return do_batch(ctx_list);

	/* Use first ctx as primary source, copy this to all the rest */
	permission = glite_metadata_getPermission(ctx_list->ctx[0],item);
	if (!permission)
	{
		error("Missing permission for %s", item);
		return -1;
	}

	/* Call setPermission() only if there was a change */
	if (apply_modes(permission, cmds))
	{
		items[batchcount] = g_strdup(item);
		perms[batchcount] =
			glite_catalog_Permission_clone(ctx_list->ctx[0], permission);
		if (!items[batchcount] || !perms[batchcount])
		{
			error("eds-chmod: Out of memory");
			return -1;
		}
		batchcount++;

		if (batchcount >= batch_factor)
			ret = do_batch(ctx_list);
	}

	glite_catalog_Permission_free(NULL, permission);

	return ret;
}

/* Perform an chmod operation */
int tool_doit(glite_catalog_ctx_list *ctx_list, int argc, char *argv[])
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

	ret = EXIT_SUCCESS;
	for (i = 1; i <= argc; i++) /* last one to flush the queue */
	{
		char *arg = i < argc ? argv[i] : NULL;

		if(do_chmod(ctx_list, arg))
			ret = EXIT_FAILURE;
	}

	return ret;
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
