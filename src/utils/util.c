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
 *  GLite Data Catalog - Utility functions for the command-line tools
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: util.c,v 1.5 2010-04-07 11:11:06 jwhite Exp $ 
 *  Release: $Name: not supported by cvs2svn $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/catalog/metadata/c/metadata-simple.h>
#include <glite/data/glite-util.h>

#include <glite/data/hydra/c/eds-simple.h>
#include <glite/data/catalog/metadata/c/metadata-simple.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tool-main.h"

/**********************************************************************
 * Global variables
 */

/* Permission bit letters */
static const char *perm_bits = "pdrwlxgs";


/**********************************************************************
 * Utility functions
 */

void print_perm(GString *dst, glite_catalog_Perm perm)
{
	char buf[GLITE_CATALOG_PERM_NUMBITS + 1];
	unsigned i;

	for (i = 0; i < strlen(perm_bits); i++)
	{
		if (perm & (1 << i))
			buf[i] = perm_bits[i];
		else
			buf[i] = '-';
	}

	buf[i] = '\0';
	g_string_append(dst, buf);
}

int parse_perm(const char *src, glite_catalog_Perm *perm)
{
	char *p;

	*perm = 0;
	while (*src && *src != '\n' && *src != '\r')
	{
		if (*src == '-')
		{
			src++;
			continue;
		}
		p = strchr(perm_bits, *src);
		if (!p)
			return -1;
		*perm |= 1 << (p - perm_bits);
		src++;
	}
	return 0;
}

/* Mode format: [uoga]*(+|-|=)[pdrwlxgs]+ */
static chmod_cmd *parse_one_modespec(const char *mode)
{
	const char *op, *p;
	chmod_cmd *cmd;

	cmd = g_new(chmod_cmd, 1);

	/* Look for the operator */
	op = mode;
	while (*op && *op != '+' && *op != '-' && *op != '=')
		op++;
	switch (*op)
	{
		case '+':
			cmd->opcode = OP_MODIFY;
			break;
		case '-':
			cmd->opcode = OP_DEL;
			break;
		case '=':
			cmd->opcode = OP_SET;
			break;
		default:
			g_free(cmd);
			return NULL;
	}

	/* If there is no subject specifier, it is treated as 'a' */
	if (mode == op)
		cmd->subject = USER | GROUP | OTHER;
	else
		cmd->subject = 0;

	/* Otherwise, parse the subject specifier */
	for (p = mode; p != op; p++) switch (*p)
	{
		case 'u':
			cmd->subject |= USER;
			break;
		case 'g':
			cmd->subject |= GROUP;
			break;
		case 'o':
			cmd->subject |= OTHER;
			break;
		case 'a':
			cmd->subject = USER | GROUP | OTHER;
			break;
		default:
			g_free(cmd);
			return NULL;
	}

	/* There must be a mask after the operator */
	if (!op[1])
	{
		g_free(cmd);
		return NULL;
	}
	if (parse_perm(op + 1, &cmd->mask))
	{
		g_free(cmd);
		return NULL;
	}
	return cmd;
}

GList *parse_modespec(const char *mode)
{
	chmod_cmd *cmd;
	GString *str;
	GList *l;
	char *p;

	str = g_string_new(mode);
	p = str->str;
	l = NULL;

	while (p)
	{
		cmd = parse_one_modespec(strsep(&p, ","));
		if (!cmd)
		{
			g_list_foreach(l, (GFunc)g_free, NULL);
			g_list_free(l);
			return NULL;
		}
		l = g_list_append(l, cmd);
	}

	return l;
}

/* Apply the mask to a single glite_catalog_Perm according to the operator */
static glite_catalog_Perm apply_mask(glite_catalog_Perm *perm, chmod_cmd *cmd)
{
	glite_catalog_Perm new_perm;
	int changed;

	switch (cmd->opcode)
	{
		case OP_MODIFY:
			new_perm = *perm | cmd->mask;
			break;
		case OP_DEL:
			new_perm = *perm & ~cmd->mask;
			break;
		case OP_SET:
			new_perm = cmd->mask;
			break;
		default:
			new_perm = *perm;
			break;
	}

	changed = ( new_perm != *perm );
	*perm = new_perm;
	return changed;
}

/* Apply the mask to all requested subjects */
int apply_modes(glite_catalog_Permission *perm, GList *cmds)
{
	chmod_cmd *cmd;
	int changed;
	GList *l;

	changed = 0;
	for (l = cmds; l; l = l->next)
	{
		cmd = l->data;

		if (cmd->subject & USER)
			changed |= apply_mask(&perm->userPerm, cmd);
		if (cmd->subject & GROUP)
			changed |= apply_mask(&perm->groupPerm, cmd);
		if (cmd->subject & OTHER)
			changed |= apply_mask(&perm->otherPerm, cmd);
	}
	return changed;
}

void print_acls(glite_catalog_Permission *perms)
{
	GString *str;
	unsigned i;

	printf("# User: %s\n", perms->userName);
	printf("# Group: %s\n", perms->groupName);

	str = g_string_new("# Base perms: user ");
	print_perm(str, perms->userPerm);
	g_string_append(str, ", group ");
	print_perm(str, perms->groupPerm);
	g_string_append(str, ", other ");
	print_perm(str, perms->otherPerm);
	printf("%s\n", str->str);

	for (i = 0; i < perms->acl_cnt; i++)
	{
		g_string_truncate(str, 0);
		print_perm(str, perms->acl[i]->principalPerm);
		printf("%s:%s\n", perms->acl[i]->principal, str->str);
	}
	g_string_free(str, 1);
}

static int check_duplicate(GList *list, const char *principal)
{
	GList *l;

	for (l = list; l; l = l->next)
	{
		glite_catalog_ACLEntry *acl = l->data;

		if (!strcmp(acl->principal, principal))
			return 1;
	}

	return 0;
}

int parse_acl(acl_ctx *actx, char *opt_arg, operation_code op)
{
	glite_catalog_ACLEntry *acl;
	glite_catalog_Perm perm;
	char *p;
	int ret;

	p = strchr(opt_arg, ':');
	if (p)
	{
		ret = parse_perm(p + 1, &perm);
		if (ret)
		{
			error("setacl: Illegal permissions in %s", opt_arg);
			return -1;
		}

		/* Cut the permission part */
		*p = '\0';
	}
	else if (op == OP_DEL)
	{
		perm = 0;
	}
	else
	{
		error("setacl: Missing permissions in %s", opt_arg);
		return -1;
	}

	if (check_duplicate(actx->mod_acls, opt_arg))
	{
		error("setacl: Principal %s was already listed as to "
			"be added/modified", opt_arg);
		return -1;
	}

	if (check_duplicate(actx->del_acls, opt_arg))
	{
		error("setacl: Principal %s was already listed as to "
			"be deleted", opt_arg);
		return -1;
	}

	acl = glite_catalog_ACLEntry_new(NULL, opt_arg, perm);
	if (!acl)
	{
		error("Out of memory");
		return -1;
	}

	switch (op)
	{
		case OP_DEL:
			actx->del_acls = g_list_append(actx->del_acls, acl);
			break;
		case OP_MODIFY:
			actx->mod_acls = g_list_append(actx->mod_acls, acl);
			break;
		case OP_SET:
			actx->set_acls = g_list_append(actx->set_acls, acl);
			break;
	}

	return 0;
}

int read_aclfile(acl_ctx *actx, const char *filename, operation_code op)
{
	char buf[2048], *p;
	FILE *fp;
	int ret;

	if (filename[0] == '-' && !filename[1])
		fp = stdin;
	else
	{
		fp = fopen(filename, "r");
		if (!fp)
		{
			error("Failed to open %s", filename);
			return -1;
		}
	}

	while (fgets(buf, sizeof(buf), fp))
	{
		p = buf;
		while (*p == ' ' || *p == '\t')
			p++;

		/* Stop parsing at the first empty line */
		if (*p == '\n')
			break;
		/* Skip comment lines */
		if (*p == '#')
			continue;

		ret = parse_acl(actx, p, op);
		if (ret)
		{
			if (fp != stdin)
				fclose(fp);
			return -1;
		}
	}

	if (fp != stdin)
		fclose(fp);
	return 0;
}

int update_acls(glite_catalog_ctx *ctx, acl_ctx *actx,
	glite_catalog_Permission *perm)
{
	glite_catalog_ACLEntry *acl;
	int ret, changed;
	GList *l;

	changed = 0;

	/* Delete the unwanted ACLs */
	for (l = actx->del_acls; l; l = l->next)
	{
		acl = l->data;

		ret = glite_catalog_Permission_delACLEntry(ctx, perm,
			acl->principal);
		if (!ret)
			changed = 1;
	}

	/* Add/modify ACLs */
	for (l = actx->mod_acls; l; l = l->next)
	{
		acl = l->data;

		/* Delete any previous ACL entry for this principal */
		glite_catalog_Permission_delACLEntry(ctx, perm, acl->principal);

		ret = glite_catalog_Permission_addACLEntry(ctx, perm, acl);
		if (!ret)
			changed = 1;
		else
		{
			glite_catalog_set_error(ctx,
				GLITE_CATALOG_ERROR_UNKNOWN,
				"Failed to set ACL entry for %s: %s",
				acl->principal, glite_catalog_get_error(ctx));
			return -1;
		}
	}

	/* Set exact ACLs */
	if (actx->set_acls)
	{
		/* First step: remove existing ACLs */
		if (perm->acl_cnt)
		{
			glite_catalog_ACLEntry_freeArray(ctx, perm->acl_cnt,
				perm->acl);
			perm->acl_cnt = 0;
			perm->acl = NULL;

			changed = 1;
		}

		/* Next step: add the new ACLs */
		for (l = actx->set_acls; l; l = l->next)
		{
			acl = l->data;

			/* Special case for "delete all ACL entries" */
			if (!acl)
				break;

			ret = glite_catalog_Permission_addACLEntry(ctx, perm,
				acl);
			if (!ret)
				changed = 1;
			else
			{
				glite_catalog_set_error(ctx,
					GLITE_CATALOG_ERROR_UNKNOWN,
					"Failed to set ACL entry for %s: %s",
					acl->principal,
					glite_catalog_get_error(ctx));
				return -1;
			}
		}
	}

	return changed;
}

void acl_ctx_destroy(acl_ctx *actx)
{
	while (actx->del_acls)
	{
		glite_catalog_ACLEntry_free(NULL, actx->del_acls->data);
		actx->del_acls = g_list_delete_link(actx->del_acls,
			actx->del_acls);
	}
	while (actx->mod_acls)
	{
		glite_catalog_ACLEntry_free(NULL, actx->mod_acls->data);
		actx->mod_acls = g_list_delete_link(actx->mod_acls,
			actx->mod_acls);
	}
	while (actx->set_acls)
	{
		glite_catalog_ACLEntry_free(NULL, actx->set_acls->data);
		actx->set_acls = g_list_delete_link(actx->set_acls,
			actx->set_acls);
	}
}

/* Write some information about the service */
int print_service_info(glite_catalog_ctx *ctx)
{
	const char *endpoint;
	char *ver;

	/* this will trigger the initialization */
	ver = glite_metadata_getVersion(ctx);

	endpoint = glite_catalog_get_endpoint(ctx);
	if (!endpoint)
	{
		error("Failed to determine the endpoint: %s",
				glite_catalog_get_error(ctx));
		exit(EXIT_FAILURE);
	}
	info("# Using endpoint %s", endpoint);

	if (!ver)
	{
		error("Failed to determine the version of the "
				"service: %s", glite_catalog_get_error(ctx));
		exit(EXIT_FAILURE);
	}
	info("# - service version: %s", ver);
	free(ver);

	ver = glite_metadata_getInterfaceVersion(ctx);
	if (!ver)
	{
		error("Failed to determine the interface version of "
				"the service: %s",
				glite_catalog_get_error(ctx));
		return -1;
	}
	info("# - interface version: %s", ver);
	free(ver);

	ver = glite_metadata_getSchemaVersion(ctx);
	if (!ver)
	{
		error("Failed to determine the schema version of "
				"the service: %s",
				glite_catalog_get_error(ctx));
		return -1;
	}
	info("# - schema version: %s", ver);
	free(ver);

	ver = glite_metadata_getServiceMetadata(ctx, "feature.string");
	if (ver)
	{
		info("# - service features: %s", ver);
		free(ver);
	}

	return 0;
}

