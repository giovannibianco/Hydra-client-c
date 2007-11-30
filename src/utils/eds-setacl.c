/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Catalog - Modify ACL entries
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: eds-setacl.c,v 1.3 2007-11-30 17:47:15 szamsu Exp $ 
 *  Release: $Name: not supported by cvs2svn $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/catalog/metadata/c/metadata-simple.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include "tool-main.h"

/**********************************************************************
 * Global variables
 */

/* Usage message */
const char *tool_usage = "ITEM [ITEM...]";

/* Help message */
const char *tool_help =
	"\t-b NUM\t\tNumber of updates in a batch\n"
	"\t-d PRINCIPAL\tDelete the ACL entry for the given principal. May be\n"
	"\t\t\tspecified multiple times\n"
	"\t-D ENTRY\t\tRead the list of ACLs to be deleted from ENTRY\n"
	"\t-m ACL\t\tAdd a new ACL entry or modify an existing one. May be \n"
	"\t\t\tspecified multiple times\n"
	"\t-M ENTRY\t\tRead the list of ACLs to be added/modified from ENTRY\n"
	"\t-x ACL\t\tSet exact ACL. May be specified multiple times, but not\n"
	"\t\t\twith -m or -d\n"
	"\t-X ENTRY\t\tRead the exact list of ACLs to be set from ENTRY\n"
	"Syntax of ACL is: principal:[pdrwlxgs]+\n";

/* setacl-specific command line options */
const char *tool_options = "b:d:D:m:M:x:X:";

/* Mask of allowed operations */
static operation_code allowed = OP_DEL | OP_MODIFY | OP_SET;

/* Operation list */
static acl_ctx actx;

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
	unsigned i;

	if (!batchcount)
		return 0;

	for(i = 0; i < (unsigned)ctx_list->count; i++)
	{
		ret |= glite_metadata_setPermission_multi(ctx_list->ctx[i], batchcount,
			(const char **)items, (const glite_catalog_Permission **)perms);
	}

	for (i = 0; i < batchcount; i++)
	{
		if (ret)
			error("Change failed on %s", items[i]);
		else
			info("Changed ACLs on %s", items[i]);
		g_free(items[i]);
		glite_catalog_Permission_free(NULL, perms[i]);
	}
	batchcount = 0;

	return ret;
}

static int set_acl(glite_catalog_ctx_list *ctx_list, const char *item)
{
	glite_catalog_Permission *permission;
	int ret = 0;
	int changed;

	if (item == NULL) /* flush the queue */
		return do_batch(ctx_list);

	/* Use first ctx as primary source, copy this to all the rest */
	permission = glite_metadata_getPermission(ctx_list->ctx[0],item);
	if (!permission)
	{
		error("Permission missing for %s", item);
		return -1;
	}

	changed = update_acls(ctx_list->ctx[0], &actx, permission);
	if (changed < 0)
	{
		ret = changed;
		goto fail;
	}
	else if (changed)
	{
		/* Write back the permissions to all services but only if there was a change */
		items[batchcount] = g_strdup(item);
		perms[batchcount] =
			glite_catalog_Permission_clone(ctx_list->ctx[0],permission);
		if (!items[batchcount] || !perms[batchcount])
		{
			error("eds-setacl: Out of memory");
			ret = -1;
			goto fail;
		}
		batchcount++;

		if (batchcount >= batch_factor)
			ret = do_batch(ctx_list);
	}

fail:
	glite_catalog_Permission_free(NULL, permission);

	return 0;
}

int tool_doit(glite_catalog_ctx_list *ctx_list, int argc, char *argv[])
{
/* 	glite_catalog_exp_flag flags; */
	int i, ret;

	if (!argc)
	{
		error("eds-setacl: Missing ITEM");
		return EXIT_FAILURE;
	}

	if (!actx.mod_acls && !actx.del_acls && !actx.set_acls)
	{
		error("eds-setacl: No operations specified");
		return EXIT_FAILURE;
	}

	/* Set the default batch factor */
	if (!batch_factor)
		batch_factor = DEFAULT_BATCH_FACTOR;

	items = g_new(char *, batch_factor);
	perms = g_new(glite_catalog_Permission *, batch_factor);

	ret = EXIT_SUCCESS;
	for (i = 0; i <= argc; i++) /* last one to flush the queue */
	{
		char *arg = i < argc ? argv[i] : NULL;

		if (set_acl(ctx_list, arg))
			ret = EXIT_FAILURE;
	}

	return ret;
}

/**********************************************************************
 * Command line parsing
 */

/* Parse setacl-specific command line options */
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
		case 'd':
			if (!(allowed & OP_DEL))
			{
				error("Option -d cannot be mixed with the "
					"other options");
				return -1;
			}
			allowed &= ~OP_SET;
			return parse_acl(&actx, opt_arg, OP_DEL);
		case 'D':
			if (actx.mod_acls || actx.del_acls || actx.set_acls)
			{
				error("Option -D must be used alone");
				return -1;
			}
			allowed = 0;
			return read_aclfile(&actx, opt_arg, OP_DEL);
		case 'm':
			if (!(allowed & OP_MODIFY))
			{
				error("Option -m cannot be mixed with the "
					"other options");
				return -1;
			}
			allowed &= ~OP_SET;
			return parse_acl(&actx, opt_arg, OP_MODIFY);
		case 'M':
			if (actx.mod_acls || actx.del_acls || actx.set_acls)
			{
				error("Option -M must be used alone");
				return -1;
			}
			allowed = 0;
			return read_aclfile(&actx, opt_arg, OP_MODIFY);
		case 'x':
			if (!(allowed & OP_SET))
			{
				error("Option -x cannot be mixed with the "
					"other options");
				return -1;
			}
			allowed &= ~(OP_DEL | OP_MODIFY);

			if (!strcmp(opt_arg, "NONE"))
			{
				/* Special case for "delete all ACLs */
				if (actx.set_acls)
				{
					error("Deleting all ACLs cannot be "
						"mixed with other -x options");
					return -1;
				}
				actx.set_acls = g_list_append(actx.set_acls,
					NULL);
			}
			else
			{
				if (actx.set_acls && !actx.set_acls->data)
				{
					error("Deleting all ACLs cannot be "
						"mixed with other -x options");
					return -1;
				}

				return parse_acl(&actx, opt_arg, OP_SET);
			}
			return 0;
		case 'X':
			if (actx.mod_acls || actx.del_acls || actx.set_acls)
			{
				error("Option -X must be used alone");
				return -1;
			}
			allowed = 0;
			return read_aclfile(&actx, opt_arg, OP_SET);
		default:
			break;
	}
	return -1;
}
