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
 *  GLite Data Management - Simple Fireman API: SOAP conversion functions
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: soapconv.c,v 1.2 2010-04-07 11:11:06 jwhite Exp $
 *  Release: $Name: not supported by cvs2svn $
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/catalog/c/catalog-simple.h>

#include <glite/data/glite-util.h>

#include <glite/data/catalog/metadata/c/metadataStub.h>
#include "internal.h"

/**********************************************************************
 * Conversion to/from gSOAP data types
 */

/* Convert glite_catalog_Perm to gSOAP's glite__Perm */
struct glite__Perm *_glite_catalog_to_soap_Perm(struct soap *soap, glite_catalog_Perm perm)
{
	struct glite__Perm *sperm;

	sperm = soap_malloc(soap, sizeof(*sperm));
	if (!soap)
		return NULL;
	memset(sperm, 0, sizeof(*sperm));

	sperm->permission = !!(perm & GLITE_CATALOG_PERM_PERMISSION);
	sperm->remove = !!(perm & GLITE_CATALOG_PERM_REMOVE);
	sperm->read = !!(perm & GLITE_CATALOG_PERM_READ);
	sperm->write = !!(perm & GLITE_CATALOG_PERM_WRITE);
	sperm->list = !!(perm & GLITE_CATALOG_PERM_LIST);
	sperm->execute = !!(perm & GLITE_CATALOG_PERM_EXECUTE);
	sperm->getMetadata = !!(perm & GLITE_CATALOG_PERM_GETMETADATA);
	sperm->setMetadata = !!(perm & GLITE_CATALOG_PERM_SETMETADATA);
	return sperm;
}

/* Convert gSOAP's glite__Perm to glite_catalog_Perm */
static glite_catalog_Perm from_soap_Perm(const struct glite__Perm *sperm)
{
	glite_catalog_Perm perm;

	/* Default: no access */
	perm = 0;

	if (!sperm)
		return perm;
	if (sperm->permission)
		perm |= GLITE_CATALOG_PERM_PERMISSION;
	if (sperm->remove)
		perm |= GLITE_CATALOG_PERM_REMOVE;
	if (sperm->read)
		perm |= GLITE_CATALOG_PERM_READ;
	if (sperm->write)
		perm |= GLITE_CATALOG_PERM_WRITE;
	if (sperm->list)
		perm |= GLITE_CATALOG_PERM_LIST;
	if (sperm->execute)
		perm |= GLITE_CATALOG_PERM_EXECUTE;
	if (sperm->getMetadata)
		perm |= GLITE_CATALOG_PERM_GETMETADATA;
	if (sperm->setMetadata)
		perm |= GLITE_CATALOG_PERM_SETMETADATA;

	return perm;
}

/* Convert glite_catalog_ACLEntry to gSOAP's glite__ACLEntry */
static struct glite__ACLEntry *to_soap_ACLEntry(struct soap *soap,
	const glite_catalog_ACLEntry *acl)
{
	struct glite__ACLEntry *sacl;

	sacl = soap_malloc(soap, sizeof(*sacl));
	if (!sacl)
		return NULL;
	memset(sacl, 0, sizeof(*sacl));

	sacl->principal = soap_strdup(soap, acl->principal);
	sacl->principalPerm = _glite_catalog_to_soap_Perm(soap, acl->principalPerm);
	if (!sacl->principal || !sacl->principalPerm)
		return NULL;
	return sacl;
}

/* Convert gSOAP's glite__ACLEntry to glite_catalog_ACLEntry */
static glite_catalog_ACLEntry *from_soap_ACLEntry(glite_catalog_ctx *ctx,
	const struct glite__ACLEntry *sacl)
{
	glite_catalog_ACLEntry *acl;
	glite_catalog_Perm perm;

	if (!sacl)
	{
		err_invarg(ctx, "SOAP returned NULL ACL");
		return NULL;
	}

	perm = from_soap_Perm(sacl->principalPerm);
	acl = glite_catalog_ACLEntry_new(ctx, sacl->principal, perm);
	return acl;
}

/* Convert glite_catalog_Permission to gSOAP's glite__Permission */
struct glite__Permission *_glite_catalog__glite_catalog_to_soap_Permission(struct soap *soap,
	const glite_catalog_Permission *permission)
{
	struct glite__Permission *spermission;
	int i;

	spermission = soap_malloc(soap, sizeof(*spermission));
	if (!spermission)
		return NULL;
	memset(spermission, 0, sizeof(*spermission));

	spermission->userName = soap_strdup(soap, permission->userName);
	spermission->groupName = soap_strdup(soap, permission->groupName);
	spermission->userPerm = _glite_catalog_to_soap_Perm(soap, permission->userPerm);
	spermission->groupPerm = _glite_catalog_to_soap_Perm(soap, permission->groupPerm);
	spermission->otherPerm = _glite_catalog_to_soap_Perm(soap, permission->otherPerm);

	if ((permission->userName && !spermission->userName) ||
			(permission->groupName && !spermission->groupName) ||
			!spermission->userPerm ||
			!spermission->groupPerm ||
			!spermission->otherPerm)
		return NULL;

	spermission->__sizeacl = permission->acl_cnt;
	if (!spermission->__sizeacl)
	{
		spermission->acl = NULL;
		return spermission;
	}

	spermission->acl = soap_malloc(soap,
		spermission->__sizeacl * sizeof(*spermission->acl));
	if (!spermission->acl)
		return NULL;

	for (i = 0; i < spermission->__sizeacl; i++)
	{
		spermission->acl[i] = to_soap_ACLEntry(soap, permission->acl[i]);
		if (!spermission->acl[i])
			return NULL;
	}

	return spermission;
}

/* Convert gSOAP's glite__Permission to glite_catalog_Permission */
glite_catalog_Permission *_glite_catalog_from_soap_Permission(glite_catalog_ctx *ctx,
	const struct glite__Permission *spermission)
{
	glite_catalog_Permission *permission;
	int i, ret;

	if (!spermission)
		return NULL;

	permission = glite_catalog_Permission_new(ctx);
	if (!permission)
		return NULL;
	ret = glite_catalog_Permission_setUserName(ctx, permission,
		spermission->userName);
	ret |= glite_catalog_Permission_setGroupName(ctx, permission,
		spermission->groupName);
	if (ret)
	{
		glite_catalog_Permission_free(ctx, permission);
		return NULL;
	}

	/* Convert the permission bits */
	permission->userPerm = from_soap_Perm(spermission->userPerm);
	permission->groupPerm = from_soap_Perm(spermission->groupPerm);
	permission->otherPerm = from_soap_Perm(spermission->otherPerm);

	/* Convert the ACL list */
	for (i = 0; i < spermission->__sizeacl; i++)
	{
		glite_catalog_ACLEntry *acl;

		acl = from_soap_ACLEntry(ctx, spermission->acl[i]);
		if (acl)
		{
			ret = glite_catalog_Permission_addACLEntry(ctx,
				permission, acl);
			glite_catalog_ACLEntry_free(ctx, acl);
			if (ret)
			{
				glite_catalog_Permission_free(ctx, permission);
				return NULL;
			}
		}
	}

	return permission;
}

/* Convert glite_catalog_Attribute to gSOAP's glite__Attribute */
struct glite__Attribute *_glite_catalog_to_soap_Attribute(struct soap *soap,
	const glite_catalog_Attribute *attr)
{
	struct glite__Attribute *sattr;

	sattr = soap_malloc(soap, sizeof(*sattr));
	if (!sattr)
		return NULL;
	memset(sattr, 0, sizeof(*sattr));

	sattr->name = soap_strdup(soap, attr->name);
	sattr->value = soap_strdup(soap, attr->value);
	sattr->type = soap_strdup(soap, attr->type);
	if (!sattr->name || (attr->name && !sattr->value) ||
			(attr->type && !sattr->type))
		return NULL;
	return sattr;
}

/* Convert gSOAP's glite__Attribute to glite_catalog_Attribute */
glite_catalog_Attribute *_glite_catalog_from_soap_Attribute(glite_catalog_ctx *ctx,
	const struct glite__Attribute *sattr)
{
	glite_catalog_Attribute *attr;

	attr = calloc(1, sizeof(*attr));
	if (!attr)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	if (!sattr->name)
	{
		glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_SOAP,
			"Service sent empty attribute name");
		return NULL;
	}

	attr->name = strdup(sattr->name);
	if (sattr->value)
		attr->value = strdup(sattr->value);
	if (sattr->type)
		attr->type = strdup(sattr->type);
	if (!attr->name || (sattr->value && !attr->value) ||
			(sattr->type && !attr->type))
	{
		err_outofmemory(ctx);
		glite_catalog_Attribute_free(ctx, attr);
		return NULL;
	}

	return attr;
}

