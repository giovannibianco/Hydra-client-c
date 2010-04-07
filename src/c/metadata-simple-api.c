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
 *  GLite Data Management - Simple Metadata API
 *
 *  Authors: 
 *      Gabor Gombas <Gabor.Gombas@cern.ch>
 *      Akos Frohner <Akos.Frohner@cern.ch>
 *      Ricardo Rocha <Ricardo.Rocha@cern.ch>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/catalog/metadata/c/metadata-simple.h>

#include <glite/data/catalog/metadata/c/metadata.nsmap>
#include <cgsi_plugin.h>

#include <glite/data/glite-util.h>

#include "metadata_internal.h"

#include <alloca.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

/**********************************************************************
 * Local helper functions
 */

/* Decode an exception inside a SOAP fault */
static void decode_exception(glite_catalog_ctx *ctx,
	struct SOAP_ENV__Detail *detail, const char *method)
{
	char *message;

	if (!detail)
		return;

#if _GSOAP_VERSION >= 0x020700
#define SET_EXCEPTION(exc, err) \
	message = ((struct glite__ ## exc *)detail->fault)->message; \
	if (!message) \
		message = #exc " received from the service"; \
	glite_catalog_set_error(ctx, GLITE_CATALOG_EXCEPTION_ ## err, \
		"%s: %s", method, message);
#define SET_EXCEPTION2(exc, err) \
	message = (NULL != ((struct metadata__ ## exc *)detail->fault)->fault)?((struct metadata__ ## exc *)detail->fault)->fault->message:NULL; \
	if (!message) \
		message = #exc " received from the service"; \
	glite_catalog_set_error(ctx, GLITE_CATALOG_EXCEPTION_ ## err, \
		"%s: %s", method, message);
#else
#define SET_EXCEPTION(exc, err) \
	message = ((struct glite__ ## exc *)detail->value)->message; \
	if (!message) \
		message = #exc " received from the service"; \
	glite_catalog_set_error(ctx, GLITE_CATALOG_EXCEPTION_ ## err, \
		"%s: %s", method, message);
#define SET_EXCEPTION2(exc, err) \
	message = (NULL != ((struct metadata__ ## exc *)detail->value)->fault)?((struct metadata__ ## exc *)detail->value)->fault->message:NULL; \
	if (!message) \
		message = #exc " received from the service"; \
	glite_catalog_set_error(ctx, GLITE_CATALOG_EXCEPTION_ ## err, \
		"%s: %s", method, message);
#endif

	switch (detail->__type)
	{
		case SOAP_TYPE_glite__CatalogException:
			SET_EXCEPTION(CatalogException, CATALOG);
			break;
		case SOAP_TYPE_glite__InternalException:
			SET_EXCEPTION(InternalException, INTERNAL);
			break;
		case SOAP_TYPE_glite__AuthorizationException:
			SET_EXCEPTION(AuthorizationException, AUTHORIZATION);
			break;
		case SOAP_TYPE_glite__NotExistsException:
			SET_EXCEPTION(NotExistsException, NOTEXISTS);
			break;
		case SOAP_TYPE_glite__InvalidArgumentException:
			SET_EXCEPTION(InvalidArgumentException, INVALIDARGUMENT);
			break;
		case SOAP_TYPE_glite__ExistsException:
			SET_EXCEPTION(ExistsException, EXISTS);
			break;
#if _GSOAP_VERSION < 0x020709
		case SOAP_TYPE_metadata__InternalException:
			SET_EXCEPTION2(InternalException, INTERNAL);
			break;
		case SOAP_TYPE_metadata__AuthorizationException:
			SET_EXCEPTION2(AuthorizationException, AUTHORIZATION);
			break;
		case SOAP_TYPE_metadata__NotExistsException:
			SET_EXCEPTION2(NotExistsException, NOTEXISTS);
			break;
		case SOAP_TYPE_metadata__InvalidArgumentException:
			SET_EXCEPTION2(InvalidArgumentException, INVALIDARGUMENT);
			break;
		case SOAP_TYPE_metadata__ExistsException:
			SET_EXCEPTION2(ExistsException, EXISTS);
			break;
#endif
		default:
			/* Let the generic error decoding handle this */
			break;
	}
#undef SET_EXCEPTION
#undef SET_EXCEPTION2
}

static int get_int_prop(glite_catalog_ctx *ctx, const char *name)
{
	char *prop, *p;
	int value;

	prop = glite_metadata_getServiceMetadata(ctx, name);
	if (!prop)
	{
		if (ctx->errclass != GLITE_CATALOG_EXCEPTION_NOTEXISTS)
			return -1;
		return 0;
	}

	value = strtol(prop, &p, 10);
	if (p && *p)
	{
		glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_UNKNOWN,
			"Service returned illegal value %s for %s", prop,
			name);
		free(prop);
		return -1;
	}
	free(prop);
	return value;
}

static int update_interface_version(glite_catalog_ctx *ctx)
{
	struct metadata__getInterfaceVersionResponse resp;
	char *version;
	int ret;

	/* Make the SOAP call */
	ret = soap_call_metadata__getInterfaceVersion(ctx->soap, ctx->endpoint,
		NULL, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "getInterfaceVersion");
		return -1;
	}

	if (!resp.getInterfaceVersionReturn)
	{
		glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_SOAP,
			"getInterfaceVersion: Service sent empty interface "
			"version");
		soap_end(ctx->soap);
		return -1;
	}

	version = strdup(resp.getInterfaceVersionReturn);
	soap_end(ctx->soap);
	if (!version)
	{
		err_outofmemory(ctx);
		return -1;
	}

	/* free it, in case it was initialized */
	free(ctx->interface_version);
	ctx->interface_version = version;
	return 0;
}

/* Check if the context strcuture is usable */
static int is_ctx_ok(glite_catalog_ctx *ctx)
{
	int ret;

	/* A NULL context is not usable */
	if (!ctx)
		return FALSE;

    /* If the context is already initialized for the metadata port,
	 * do nothing */
	if (ctx->port_type == GLITE_CATALOG_PORT_METADATA)
		return TRUE;
	/* If the context is already initialized for some other port,
	 * bail out */
	if (ctx->port_type != GLITE_CATALOG_PORT_NONE)
		return FALSE;

    /* overriding the "virtual methods" */
    ctx->decode_exception = &decode_exception;

    if (getenv(GLITE_METADATA_SD_ENV))
	    ret = _glite_catalog_init_endpoint(ctx, metadata_namespaces,
        	    getenv(GLITE_METADATA_SD_ENV));
    else
	    ret = _glite_catalog_init_endpoint(ctx, metadata_namespaces,
        	    GLITE_METADATA_SD_TYPE);
    if (ret)
        return FALSE;

    ret = update_interface_version(ctx);
    if (ret)
        return FALSE;

    ctx->port_type = GLITE_CATALOG_PORT_METADATA;
	return TRUE;
}

/**********************************************************************
 * Library interface functions
 */

int glite_metadata_get_query_limit(glite_catalog_ctx *ctx)
{
	if (!is_ctx_ok(ctx))
		return -1;

	if (ctx->query_limit > 0)
		return ctx->query_limit;

	return get_int_prop(ctx, "limit.query");
}

/**********************************************************************
 * Service requests
 *
 * Interface: org.glite.data.catalog.service.ServiceBase
 */

char *glite_metadata_getVersion(glite_catalog_ctx *ctx)
{
	struct metadata__getVersionResponse resp;
	char *version;
	int ret;

	if (!is_ctx_ok(ctx))
		return NULL;

	/* Make the SOAP call */
	ret = soap_call_metadata__getVersion(ctx->soap, ctx->endpoint, NULL,
		&resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "getVersion");
		return NULL;
	}

	if (resp.getVersionReturn)
		version = strdup(resp.getVersionReturn);
	else
		version = NULL;
	soap_end(ctx->soap);
	return version;
}

char *glite_metadata_getSchemaVersion(glite_catalog_ctx *ctx)
{
	struct metadata__getSchemaVersionResponse resp;
	char *version;
	int ret;

	if (!is_ctx_ok(ctx))
		return NULL;

	/* Make the SOAP call */
	ret = soap_call_metadata__getSchemaVersion(ctx->soap, ctx->endpoint,
		NULL, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "getSchemaVersion");
		return NULL;
	}

	if (resp.getSchemaVersionReturn)
		version = strdup(resp.getSchemaVersionReturn);
	else
		version = NULL;
	soap_end(ctx->soap);
	return version;
}

char *glite_metadata_getInterfaceVersion(glite_catalog_ctx *ctx)
{
	char *version;

	if (!is_ctx_ok(ctx))
		return NULL;

	/* Use the cached version */
	version = strdup(ctx->interface_version);
	if (!version)
		err_outofmemory(ctx);

	return version;
}

char *glite_metadata_getServiceMetadata(glite_catalog_ctx *ctx, const char *key)
{
	struct metadata__getServiceMetadataResponse resp;
	char *req_key, *result;
	int ret;

	if (!is_ctx_ok(ctx))
		return NULL;
	if (!key)
	{
		err_invarg(ctx, "getServiceMetadata: Key is missing");
		return NULL;
	}

	/* We cannot turn a const pointer to a non-const, so we have to copy. */
	req_key = soap_strdup(ctx->soap, key);
	if (!req_key)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__getServiceMetadata(ctx->soap, ctx->endpoint,
		NULL, req_key, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "getServiceMetadata");
		return NULL;
	}

	if (resp._getServiceMetadataReturn)
		result = strdup(resp._getServiceMetadataReturn);
	else
		result = NULL;
	soap_end(ctx->soap);
	return result;
}

/**********************************************************************
 * Interface: org.glite.data.catalog.service.fas.FASBase
 */

int glite_metadata_setPermission(glite_catalog_ctx *ctx, const char *item,
	const glite_catalog_Permission *permission)
{
	return glite_metadata_setPermission_multi(ctx, 1, &item, &permission);
}

int glite_metadata_setPermission_multi(glite_catalog_ctx *ctx, int nitems,
	const char * const items[],
	const glite_catalog_Permission * const perm[])
{
	struct metadataArrayOf_USCOREtns1_USCOREPermissionEntry req;
	int i, ret;

	if (!is_ctx_ok(ctx))
		return -1;
	if (nitems < 1)
	{
		err_invarg(ctx, "setPermission: Illegal item number");
		return -1;
	}

	/* Build the SOAP request structure */
	req.__size = nitems;
	req.__ptr = soap_malloc(ctx->soap, nitems * sizeof(*req.__ptr));
	if (!req.__ptr)
	{
		err_outofmemory(ctx);
		return -1;
	}
	for (i = 0; i < nitems; i++)
	{
		/* Check for NULL items */
		if (!items[i])
		{
			err_soap(ctx, "setPermission: LFN is missing");
			soap_end(ctx->soap);
			return -1;
		}

		/* Check for NULL permissions */
		if (!perm[i])
		{
			err_soap(ctx, "setPermission: Permission is missing");
			soap_end(ctx->soap);
			return -1;
		}

		req.__ptr[i] = soap_malloc(ctx->soap, sizeof(*req.__ptr[i]));
		if (!req.__ptr[i])
		{
			err_outofmemory(ctx);
			soap_end(ctx->soap);
			return -1;
		}

		req.__ptr[i]->item = soap_strdup(ctx->soap, items[i]);
		req.__ptr[i]->permission = _glite_catalog__glite_catalog_to_soap_Permission(ctx->soap,
			perm[i]);
		if (!req.__ptr[i]->item || !req.__ptr[i]->permission)
		{
			err_outofmemory(ctx);
			soap_end(ctx->soap);
			return -1;
		}
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__setPermission(ctx->soap, ctx->endpoint, NULL,
		&req, NULL);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "setPermission");
		return -1;
	}

	soap_end(ctx->soap);
	return 0;
}

glite_catalog_Permission *glite_metadata_getPermission(glite_catalog_ctx *ctx,
	const char *item)
{
	glite_catalog_Permission *perm, **tmp;

	if (!item)
	{
		err_invarg(ctx, "getPermission: LFN is missing");
		return NULL;
	}
	tmp = glite_metadata_getPermission_multi(ctx, 1, &item);
	if (!tmp)
		return NULL;
	perm = tmp[0];
	free(tmp);
	return perm;
}

glite_catalog_Permission **glite_metadata_getPermission_multi(glite_catalog_ctx *ctx,
	int nitems, const char * const items[])
{
	struct metadataArrayOf_USCOREsoapenc_USCOREstring req;
	struct metadata__getPermissionResponse resp;
	struct glite__PermissionEntry **perms;
	glite_catalog_Permission **result;
	int i, itemptr, ret;

	if (!is_ctx_ok(ctx))
		return NULL;
	if (nitems < 1)
	{
		err_invarg(ctx, "getPermission: Illegal item number");
		return NULL;
	}

	/* Build the request structure */
	ret = _glite_catalog_to_soap_StringArray(ctx->soap, &req, nitems, items);
	if (ret)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return NULL;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__getPermission(ctx->soap, ctx->endpoint, NULL,
		&req, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "getPermission");
		return NULL;
	}

	/* Do a sanity check on the result */
	perms = resp._getPermissionReturn->__ptr;
	for (i = 0; i < resp._getPermissionReturn->__size; i++)
	{
		if (!perms[i]->item)
		{
			glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_SOAP,
				"getPermission: Service sent empty item name");
			soap_end(ctx->soap);
			return NULL;
		}
	}

	/* Convert the response to the result */
	result = calloc(nitems, sizeof(*result));
	if (!result)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return NULL;
	}

	/* Catch: nobody said that the answer records are in the same order
	 * as the query was, so we need to sort them. Also, we may not have
	 * the same number of results as queries */
	/* XXX Quadratic algorithm */
	if (resp._getPermissionReturn)
	{
		for (itemptr = 0; itemptr < nitems; itemptr++)
		{
			for (i = 0; i < resp._getPermissionReturn->__size; i++)
			{
				if (!strcmp(items[itemptr], perms[i]->item))
					break;
			}

			if (i >= resp._getPermissionReturn->__size)
				/* No response for this query */
				continue;

			result[itemptr] = _glite_catalog_from_soap_Permission(ctx,
				perms[i]->permission);
			if (!result[itemptr])
			{
				glite_catalog_Permission_freeArray(ctx, itemptr,
					result);
				soap_end(ctx->soap);
				return NULL;
			}
		}
	}

	soap_end(ctx->soap);
	return result;
}

int glite_metadata_checkPermission(glite_catalog_ctx *ctx, const char *item,
	glite_catalog_Perm perm)
{
	if (!item)
	{
		err_invarg(ctx, "checkPermission: LFN is missing");
		return -1;
	}
	return glite_metadata_checkPermission_multi(ctx, 1, &item, perm);
}

int glite_metadata_checkPermission_multi(glite_catalog_ctx *ctx, int nitems,
	const char * const items[], glite_catalog_Perm perm)
{
	struct metadataArrayOf_USCOREsoapenc_USCOREstring req;
	struct glite__Perm *sperm;
	int ret;

	if (!is_ctx_ok(ctx))
		return -1;
	if (nitems < 1)
	{
		err_invarg(ctx, "checkPermission: Illegal item number");
		return -1;
	}

	/* Build the request structure */
	ret = _glite_catalog_to_soap_StringArray(ctx->soap, &req, nitems, items);
	if (ret)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return -1;
	}

	sperm = _glite_catalog_to_soap_Perm(ctx->soap, perm);
	if (!sperm)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return -1;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__checkPermission(ctx->soap, ctx->endpoint, NULL,
		&req, sperm, NULL);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "checkPermission");
		return -1;
	}

	soap_end(ctx->soap);
	return 0;
}

/**********************************************************************
 * Interface: org.glite.data.catalog.service.meta.MetadataBase
 */

int glite_metadata_setAttributes(glite_catalog_ctx *ctx, const char *item,
	int nattributes, const glite_catalog_Attribute * const attributes[])
{
	struct metadataArrayOf_USCOREtns1_USCOREAttribute req;
	char *sitem;
	int i, ret;

	if (!is_ctx_ok(ctx))
		return -1;
	if (nattributes < 1)
	{
		err_invarg(ctx, "setAttributes: Illegal attribute number");
		return -1;
	}
	if (!item)
	{
		err_invarg(ctx, "setAttributes: Item is missing");
		return -1;
	}

	/* Build the request structure */
	sitem = soap_strdup(ctx->soap, item);
	if (!sitem)
	{
		err_outofmemory(ctx);
		return -1;
	}

	req.__size = nattributes;
	req.__ptr = soap_malloc(ctx->soap, nattributes * sizeof(*req.__ptr));
	if (!req.__ptr)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return -1;
	}

	for (i = 0; i < nattributes; i++)
	{
		req.__ptr[i] = _glite_catalog_to_soap_Attribute(ctx->soap, attributes[i]);
		if (!req.__ptr[i])
		{
			err_outofmemory(ctx);
			soap_end(ctx->soap);
			return -1;
		}
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__setAttributes(ctx->soap, ctx->endpoint, NULL,
		sitem, &req, NULL);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "setAttributes");
		return -1;
	}

	soap_end(ctx->soap);
	return 0;
}

int glite_metadata_clearAttributes(glite_catalog_ctx *ctx, const char *item,
	int nattributes, const char * const attributes[])
{
	struct metadataArrayOf_USCOREsoapenc_USCOREstring req;
	char *sitem;
	int ret;

	if (!is_ctx_ok(ctx))
		return -1;
	if (nattributes < 1)
	{
		err_invarg(ctx, "clearAttributes: Illegal attribute number");
		return -1;
	}
	if (!item)
	{
		err_invarg(ctx, "clearAttributes: Item is missing");
		return -1;
	}

	/* Build the request structure */
	sitem = soap_strdup(ctx->soap, item);
	if (!sitem)
	{
		err_outofmemory(ctx);
		return -1;
	}

	ret = _glite_catalog_to_soap_StringArray(ctx->soap, &req, nattributes, attributes);
	if (ret)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return -1;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__clearAttributes(ctx->soap, ctx->endpoint, NULL,
		sitem, &req, NULL);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "clearAttributes");
		return -1;
	}

	soap_end(ctx->soap);
	return 0;
}

glite_catalog_Attribute **glite_metadata_getAttributes(glite_catalog_ctx *ctx,
	const char *item, int nattributes, const char * const attributes[],
	int *resultCount)
{
	struct metadataArrayOf_USCOREsoapenc_USCOREstring req;
	struct metadata__getAttributesResponse resp;
	glite_catalog_Attribute **result;
	struct glite__Attribute **attrs;
	char *sitem;
	int i, ret;

	/* Default: error */
	if (resultCount)
		*resultCount = -1;

	if (!is_ctx_ok(ctx))
		return NULL;
	if (nattributes < 1)
	{
		err_invarg(ctx, "getAttributes: Illegal attribute number");
		return NULL;
	}
	if (!item)
	{
		err_invarg(ctx, "getAttributes: Item is missing");
		return NULL;
	}

	/* Build the request structure */
	sitem = soap_strdup(ctx->soap, item);
	if (!sitem)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	ret = _glite_catalog_to_soap_StringArray(ctx->soap, &req, nattributes, attributes);
	if (ret)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return NULL;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__getAttributes(ctx->soap, ctx->endpoint, NULL,
		sitem, &req, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "getAttributes");
		return NULL;
	}

	/* Server may have sent empty reply */
	if (!resp._getAttributesReturn)
	{
		err_soap(ctx, "getAttributes: Server sent empty reply");
		soap_end(ctx->soap);
		if (resultCount)
			*resultCount = 0;
		return NULL;
	}

	/* Convert the response to the result */
	result = malloc(resp._getAttributesReturn->__size *
		sizeof(*result));
	if (!result)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return NULL;
	}

	attrs = resp._getAttributesReturn->__ptr;
	for (i = 0; i < resp._getAttributesReturn->__size; i++)
	{
		result[i] = _glite_catalog_from_soap_Attribute(ctx, attrs[i]);
		if (!result[i])
		{
			glite_catalog_Attribute_freeArray(ctx, i, result);
			soap_end(ctx->soap);
			return NULL;
		}
	}

	*resultCount = resp._getAttributesReturn->__size;

	soap_end(ctx->soap);
	return result;
}

glite_catalog_Attribute **glite_metadata_listAttributes(glite_catalog_ctx *ctx,
	const char *item, int *resultCount)
{
	struct metadata__listAttributesResponse resp;
	glite_catalog_Attribute **result;
	struct glite__Attribute **attrs;
	char *sitem;
	int i, ret;

	/* Default: error */
	if (resultCount)
		*resultCount = -1;

	if (!is_ctx_ok(ctx))
		return NULL;
	if (!item)
	{
		err_invarg(ctx, "listAttributes: Item is missing");
		return NULL;
	}

	/* Build the request structure */
	sitem = soap_strdup(ctx->soap, item);
	if (!sitem)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__listAttributes(ctx->soap, ctx->endpoint, NULL,
		sitem, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "listAttributes");
		return NULL;
	}

	/* The response may be empty */
	if (!resp._listAttributesReturn)
	{
		if (resultCount)
			*resultCount = 0;
		soap_end(ctx->soap);
		return NULL;
	}

	/* Convert the response to the result */
	result = malloc(resp._listAttributesReturn->__size *
		sizeof(*result));
	if (!result)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return NULL;
	}

	attrs = resp._listAttributesReturn->__ptr;
	for (i = 0; i < resp._listAttributesReturn->__size; i++)
	{
		result[i] = _glite_catalog_from_soap_Attribute(ctx, attrs[i]);
		if (!result[i])
		{
			glite_catalog_Attribute_freeArray(ctx, i, result);
			soap_end(ctx->soap);
			return NULL;
		}
	}

	if (resultCount)
		*resultCount = resp._listAttributesReturn->__size;
	soap_end(ctx->soap);
	return result;
}

char **glite_metadata_query(glite_catalog_ctx *ctx, const char *query,
	const char *type, int limit, int offset, int *resultCount)
{
	struct metadata__queryResponse resp;
	char *squery, *stype, **result;
	int ret;

	/* Default: error */
	if (resultCount)
		*resultCount = -1;

	if (!is_ctx_ok(ctx))
		return NULL;
	if (!query)
	{
		err_invarg(ctx, "query: Query is missing");
		return NULL;
	}
	if (!type)
	{
		err_invarg(ctx, "query: Query type is missing");
		return NULL;
	}

	/* Build the request */
	squery = soap_strdup(ctx->soap, query);
	if (!squery)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	stype = soap_strdup(ctx->soap, type);
	if (!type)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__query(ctx->soap, ctx->endpoint, NULL,
		squery, stype, limit, offset, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "query");
		return NULL;
	}

	/* The response may be empty */
	if (!resp._queryReturn)
	{
		if (resultCount)
			*resultCount = 0;
		soap_end(ctx->soap);
		return NULL;
	}

	result = _glite_catalog_from_soap_StringArray(ctx, resp._queryReturn, resultCount);
	soap_end(ctx->soap);
	return result;
}

/**********************************************************************
 * Interface: org.glite.data.catalog.service.meta.MetadataCatalog
 */

int glite_metadata_createEntry(glite_catalog_ctx *ctx, const char *item,
	const char *schema)
{
	const char **tmp[2];

	if (!item || !schema)
	{
		err_invarg(ctx, "createEntry: ITEM or SCHEMA is missing");
		return -1;
	}

	tmp[0] = g_new0(char *, 1);
	tmp[1] = g_new0(char *, 1);
	tmp[0][0] = item;
	tmp[1][0] = schema;
	return glite_metadata_createEntry_multi(ctx, 1, tmp);
}

int glite_metadata_createEntry_multi(glite_catalog_ctx *ctx, int nitems,
	const char **items[2])
{
	const char *real_items[nitems][2];
	struct metadataArrayOf_USCOREtns1_USCOREStringPair req;
	int ret, i;

	for (i = 0; i < nitems; i++)
	{
		real_items[i][0] = items[0][i];
		real_items[i][1] = items[1][i];
	}

	if (!is_ctx_ok(ctx))
		return -1;
	if (nitems < 1)
	{
		err_invarg(ctx, "createEntry: Illegal item number");
		return -1;
	}

	/* Build the request structure */
	ret = _glite_catalog_to_soap_StringPairArray(ctx->soap, &req, nitems, real_items);
	if (ret)
	{
		err_outofmemory(ctx);
		return -1;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__createEntry(ctx->soap, ctx->endpoint,
		NULL, &req, NULL);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "createEntry");
		return -1;
	}

	soap_end(ctx->soap);
	return 0;
}

int glite_metadata_removeEntry(glite_catalog_ctx *ctx, const char *item)
{
	if (!item)
	{
		err_invarg(ctx, "removeEntry: ITEM is missing");
		return -1;
	}

	return glite_metadata_removeEntry_multi(ctx, 1, &item);
}

int glite_metadata_removeEntry_multi(glite_catalog_ctx *ctx,
	int nitems, const char * const items[])
{
	struct metadataArrayOf_USCOREsoapenc_USCOREstring req;
	int ret;

	if (!is_ctx_ok(ctx))
		return -1;
	if (nitems < 1)
	{
		err_invarg(ctx, "removeEntry: Illegal item number");
		return -1;
	}

	/* Build the request structure */
	ret = _glite_catalog_to_soap_StringArray(ctx->soap, &req, nitems, items);
	if (ret)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return -1;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__removeEntry(ctx->soap, ctx->endpoint, NULL,
		&req, NULL);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "removeEntry");
		return -1;
	}

	soap_end(ctx->soap);
	return 0;
}

/**********************************************************************
 * Interface: org.glite.data.catalog.service.meta.MetadataSchema
 */

glite_catalog_Attribute **glite_metadata_describeSchema(glite_catalog_ctx *ctx,
	const char *schema, int *resultCount)
{
	struct metadata__describeSchemaResponse resp;
	glite_catalog_Attribute **result;
	struct glite__Attribute **attrs;
	char *sitem;
	int i, ret;

	/* Default: error */
	if (resultCount)
		*resultCount = -1;

	if (!is_ctx_ok(ctx))
		return NULL;
	if (!schema)
	{
		err_invarg(ctx, "describeSchema: Schema is missing");
		return NULL;
	}

	/* Build the request structure */
	sitem = soap_strdup(ctx->soap, schema);
	if (!sitem)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	/* Make the SOAP call */
	ret = soap_call_metadata__describeSchema(ctx->soap, ctx->endpoint, NULL,
		sitem, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "describeSchema");
		return NULL;
	}

	/* The response may be empty */
	if (!resp._describeSchemaReturn)
	{
		if (resultCount)
			*resultCount = 0;
		soap_end(ctx->soap);
		return NULL;
	}

	/* Convert the response to the result */
	result = malloc(resp._describeSchemaReturn->__size *
		sizeof(*result));
	if (!result)
	{
		err_outofmemory(ctx);
		soap_end(ctx->soap);
		return NULL;
	}

	attrs = resp._describeSchemaReturn->__ptr;
	for (i = 0; i < resp._describeSchemaReturn->__size; i++)
	{
		result[i] = _glite_catalog_from_soap_Attribute(ctx, attrs[i]);
		if (!result[i])
		{
			glite_catalog_Attribute_freeArray(ctx, i, result);
			soap_end(ctx->soap);
			return NULL;
		}
	}

	if (resultCount)
		*resultCount = resp._describeSchemaReturn->__size;
	soap_end(ctx->soap);
	return result;
}



char **glite_metadata_listSchemas(glite_catalog_ctx *ctx,
	int *resultCount)
{
	struct metadata__listSchemasResponse resp;
	char **result;
	int ret;

	/* Default: error */
	if (resultCount)
		*resultCount = -1;

	if (!is_ctx_ok(ctx))
		return NULL;

	/* Make the SOAP call */
	ret = soap_call_metadata__listSchemas(ctx->soap, ctx->endpoint,
		NULL, &resp);
	if (ret != SOAP_OK)
	{
		_glite_catalog_fault_to_error(ctx, "listSchemas");
		return NULL;
	}

	/* The response may be empty */
	if (!resp.listSchemasReturn)
	{
		if (resultCount)
			*resultCount = 0;
		soap_end(ctx->soap);
		return NULL;
	}

	/* Convert the response to the result */
	result = _glite_catalog_from_soap_StringArray(ctx, resp.listSchemasReturn, resultCount);
	soap_end(ctx->soap);
	return result;
}
