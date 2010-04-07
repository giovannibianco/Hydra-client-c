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
 *  GLite Data Management - Simple Catalog API
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: datatypes.c,v 1.2 2010-04-07 11:11:06 jwhite Exp $ 
 *  Release: $Name: not supported by cvs2svn $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/catalog/c/catalog-simple.h>

#include <glite/data/catalog/metadata/c/metadataStub.h>
#include "internal.h"

#include <stdlib.h>
#include <string.h>


/**********************************************************************
 * Functions manipulating the Fireman data types
 */

glite_catalog_ACLEntry *glite_catalog_ACLEntry_new(glite_catalog_ctx *ctx,
	const char *principal, glite_catalog_Perm principalPerm)
{
	glite_catalog_ACLEntry *aclEntry;

	if (!principal)
	{
		err_invarg(ctx, "Principal name is missing");
		return NULL;
	}

	aclEntry = malloc(sizeof(*aclEntry));
	if (!aclEntry)
	{
		err_outofmemory(ctx);
		return NULL;
	}
	aclEntry->principal = strdup(principal);
	if (!aclEntry->principal)
	{
		err_outofmemory(ctx);
		free(aclEntry);
		return NULL;
	}
	aclEntry->principalPerm = principalPerm;

	return aclEntry;
}

void glite_catalog_ACLEntry_free(glite_catalog_ctx *ctx G_GNUC_UNUSED,
	glite_catalog_ACLEntry *entry)
{
	if (!entry)
		return;

	free(entry->principal);
	free(entry);
}

void glite_catalog_ACLEntry_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_ACLEntry *entries[])
{
	int i;

	for (i = 0; i < nitems; i++)
		glite_catalog_ACLEntry_free(ctx, entries[i]);
	free(entries);
}

glite_catalog_ACLEntry *glite_catalog_ACLEntry_clone(glite_catalog_ctx *ctx,
	const glite_catalog_ACLEntry *orig)
{
	glite_catalog_ACLEntry *clone;

	if (!orig)
		return NULL;
	clone = glite_catalog_ACLEntry_new(ctx, orig->principal,
		orig->principalPerm);
	return clone;
}

glite_catalog_Permission *glite_catalog_Permission_new(glite_catalog_ctx *ctx)
{
	glite_catalog_Permission *permission;

	permission = calloc(1, sizeof(*permission));
	if (!permission)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	permission->userPerm = ctx->defaultUserPerm;
	permission->groupPerm = ctx->defaultGroupPerm;
	permission->otherPerm = ctx->defaultOtherPerm;

	return permission;
}

void glite_catalog_Permission_free(glite_catalog_ctx *ctx G_GNUC_UNUSED,
	glite_catalog_Permission *permission)
{
	if (!permission)
		return;

	free(permission->userName);
	free(permission->groupName);
	free(permission);
}

void glite_catalog_Permission_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_Permission *permissions[])
{
	int i;

	for (i = 0; i < nitems; i++)
		glite_catalog_Permission_free(ctx, permissions[i]);
	free(permissions);
}

glite_catalog_Permission *glite_catalog_Permission_clone(glite_catalog_ctx *ctx,
	const glite_catalog_Permission *orig)
{
	glite_catalog_Permission *clone;
	unsigned i;
	int ret;

	if (!orig)
		return NULL;
	clone = glite_catalog_Permission_new(ctx);
	if (!clone)
		return NULL;

	if (orig->userName)
	{
		clone->userName = strdup(orig->userName);
		if (!clone->userName)
		{
			err_outofmemory(ctx);
			glite_catalog_Permission_free(ctx, clone);
			return NULL;
		}
	}
	if (orig->groupName)
	{
		clone->groupName = strdup(orig->groupName);
		if (!clone->groupName)
		{
			err_outofmemory(ctx);
			glite_catalog_Permission_free(ctx, clone);
			return NULL;
		}
	}

	clone->userPerm = orig->userPerm;
	clone->groupPerm = orig->groupPerm;
	clone->otherPerm = orig->otherPerm;

	for (i = 0; i < orig->acl_cnt; i++)
	{
		ret = glite_catalog_Permission_addACLEntry(ctx, clone,
			orig->acl[i]);
		if (ret)
		{
			glite_catalog_Permission_free(ctx, clone);
			return NULL;
		}
	}

	return clone;
}

int glite_catalog_Permission_addACLEntry(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission,
	const glite_catalog_ACLEntry *aclEntry)
{
	glite_catalog_ACLEntry *newEntry, **tmp;
	unsigned i;

	if (!permission)
	{
		err_invarg(ctx, "Permission is missing");
		return -1;
	}
	if (!aclEntry)
	{
		err_invarg(ctx, "ACL entry is missing");
		return -1;
	}

	/* Check for duplicates */
	for (i = 0; i < permission->acl_cnt; i++)
	{
		if (!strcmp(permission->acl[i]->principal, aclEntry->principal))
		{
			err_exists(ctx, "Duplicate principal");
			return -1;
		}
	}

	/* Make a copy of the ACL entry */
	newEntry = glite_catalog_ACLEntry_clone(ctx, aclEntry);
	if (aclEntry && !newEntry)
		return -1;

	tmp = realloc(permission->acl, (permission->acl_cnt + 1) *
		sizeof(*permission->acl));
	if (!tmp)
	{
		err_outofmemory(ctx);
		glite_catalog_ACLEntry_free(ctx, newEntry);
		return -1;
	}

	tmp[permission->acl_cnt++] = newEntry;
	permission->acl = tmp;
	return 0;
}

int glite_catalog_Permission_delACLEntry(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission, const char *principal)
{
	unsigned i;

	if (!permission)
	{
		err_invarg(ctx, "Permission is missing");
		return -1;
	}
	if (!principal)
	{
		err_invarg(ctx, "Principal name is missing");
		return -1;
	}

	/* Look for the ACL */
	for (i = 0; i < permission->acl_cnt; i++)
	{
		if (!strcmp(permission->acl[i]->principal, principal))
			break;
	}

	if (i >= permission->acl_cnt)
	{
		err_notexists(ctx, "No ACL entry for principal %s", principal);
		return -1;
	}

	glite_catalog_ACLEntry_free(ctx, permission->acl[i]);
	if (i + 1 < permission->acl_cnt)
		memmove(permission->acl + i, permission->acl + i + 1,
			(permission->acl_cnt - i - 1) *
			sizeof(*permission->acl));
	permission->acl_cnt--;

	return 0;
}

int glite_catalog_Permission_setUserName(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission, const char *userName)
{
	if (!permission)
	{
		err_invarg(ctx, "Permission is missing");
		return -1;
	}
	if (!userName)
	{
		err_invarg(ctx, "User name is missing");
		return -1;
	}

	free(permission->userName);
	permission->userName = strdup(userName);
	if (!permission->userName)
	{
		err_outofmemory(ctx);
		glite_catalog_Permission_free(ctx, permission);
		return -1;
	}

	return 0;
}

int glite_catalog_Permission_setGroupName(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission, const char *groupName)
{
	if (!permission)
	{
		err_invarg(ctx, "Permission is missing");
		return -1;
	}
	if (!groupName)
	{
		err_invarg(ctx, "Group name is missing");
		return -1;
	}

	free(permission->groupName);
	permission->groupName = strdup(groupName);
	if (!permission->groupName)
	{
		err_outofmemory(ctx);
		glite_catalog_Permission_free(ctx, permission);
		return -1;
	}

	return 0;
}

void glite_catalog_freeStringPairArray(int nitems, char *array[][2])
{
	int i;

	for (i = 0; i < nitems; i++)
	{
		free(array[i][0]);
		free(array[i][1]);
	}
	free(array);
}

glite_catalog_Attribute *glite_catalog_Attribute_new(glite_catalog_ctx *ctx,
	const char *name, const char *value, const char *type)
{
	glite_catalog_Attribute *attr;

	if (!name)
	{
		err_invarg(ctx, "Attribute name is missing");
		return NULL;
	}

	attr = malloc(sizeof(*attr));
	if (!attr)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	attr->name = strdup(name);
	if (value)
		attr->value = strdup(value);
	if (type)
		attr->type = strdup(type);

	if (!attr->name || (value && !attr->value) || (type && !attr->type))
	{
		err_outofmemory(ctx);
		glite_catalog_Attribute_free(ctx, attr);
		return NULL;
	}

	return attr;
}

void glite_catalog_Attribute_free(glite_catalog_ctx *ctx G_GNUC_UNUSED,
	glite_catalog_Attribute *attr)
{
	if (!attr)
		return;

	free(attr->name);
	free(attr->value);
	free(attr->type);
	free(attr);
}

void glite_catalog_Attribute_freeArray(glite_catalog_ctx *ctx,
	int nitems, glite_catalog_Attribute *attrs[])
{
	int i;

	for (i = 0; i < nitems; i++)
		glite_catalog_Attribute_free(ctx, attrs[i]);
	free(attrs);
}

glite_catalog_Attribute *glite_catalog_Attribute_clone(glite_catalog_ctx *ctx,
	glite_catalog_Attribute *orig)
{
	if (!orig)
		return NULL;
	return glite_catalog_Attribute_new(ctx, orig->name, orig->value,
		orig->type);
}

