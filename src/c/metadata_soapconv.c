/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Management - Simple Fireman API: SOAP conversion functions
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: metadata_soapconv.c,v 1.1 2007-11-21 10:48:06 szamsu Exp $
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
#include "metadata_internal.h"

/**********************************************************************
 * Conversion to/from gSOAP data types
 */

/* Convert a string pair list to gSOAP's metadataArrayOf_USCOREtns1_USCOREStringPair */
int _glite_catalog_to_soap_StringPairArray(struct soap *soap,
	struct metadataArrayOf_USCOREtns1_USCOREStringPair *req,
	int nitems, const char *items[][2])
{
	int i;

	req->__size = nitems;
	req->__ptr = soap_malloc(soap, nitems * sizeof(*req->__ptr));
	if (!req->__ptr)
		return -1;
	for (i = 0; i < nitems; i++)
	{
		req->__ptr[i] = soap_malloc(soap, sizeof(*req->__ptr[i]));
		if (!req->__ptr[i])
			return -1;
		req->__ptr[i]->string1 = soap_strdup(soap, items[i][0]);
		req->__ptr[i]->string2 = soap_strdup(soap, items[i][1]);
		if (!req->__ptr[i]->string1 || !req->__ptr[i]->string2)
			return -1;
	}

	return 0;
}

/* Convert a list of strings to gSOAP's metadataArrayOf_USCOREsoapenc_USCOREstring */
int _glite_catalog_to_soap_StringArray(struct soap *soap,
	struct metadataArrayOf_USCOREsoapenc_USCOREstring *req,
	int nitems, const char * const items[])
{
	int i;

	req->__size = nitems;
	req->__ptr = soap_malloc(soap, nitems * sizeof(*req->__ptr));
	if (!req->__ptr)
		return -1;
	for (i = 0; i < nitems; i++)
	{
		req->__ptr[i] = soap_strdup(soap, items[i]);
		if (!req->__ptr[i])
			return -1;
	}

	return 0;
}

/* Convert gSOAP's metadataArrayOf_USCOREsoapenc_USCOREstring to char ** */
char **_glite_catalog_from_soap_StringArray(glite_catalog_ctx *ctx,
	struct metadataArrayOf_USCOREsoapenc_USCOREstring *resp,
	int *resultCount)
{
	char **result;
	int i;

	/* The result may be empty */
	if (!resp)
	{
		if (resultCount)
			*resultCount = 0;
		return NULL;
	}

	result = malloc(resp->__size * sizeof(*result));
	if (!result)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	for (i = 0; i < resp->__size; i++)
	{
		if (resp->__ptr[i])
		{
			result[i] = strdup(resp->__ptr[i]);
			if (!result[i])
			{
				err_outofmemory(ctx);
				glite_freeStringArray(i, result);
				return NULL;
			}
		}
		else
			result[i] = NULL;
	}

	if (resultCount)
		*resultCount = resp->__size;

	return result;
}

/* Extract the second element of an array of string pairs in the order defined
 * by a source array */
char **_glite_catalog_flatten_soap_StringPairArray(glite_catalog_ctx *ctx, int nitems,
	const char * const orig_items[],
	struct metadataArrayOf_USCOREtns1_USCOREStringPair *pairs)
{
	int i, itemptr, startptr;
	char **result, *seen;

	/* Do a sanity check on the result */
	for (i = 0; i < pairs->__size; i++)
	{
		if (!pairs->__ptr[i]->string1)
		{
			glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_SOAP,
				"Service sent empty key in a string pair list");
			return NULL;
		}
	}

	result = malloc(nitems * sizeof(*result));
	if (!result)
	{
		err_outofmemory(ctx);
		return NULL;
	}

	/* Array of flags telling which elements did we already see */
	seen = calloc(pairs->__size, sizeof(*seen));
	if (!seen)
	{
		err_outofmemory(ctx);
		free(result);
		return NULL;
	}

	/* Catch: nobody said that the answer records are in the same order
	 * as the query was, so we need to sort them. Also, we may not have
	 * the same number of results as queries */
	startptr = 0;
	for (itemptr = 0; itemptr < nitems; itemptr++)
	{
		/* startptr provides linear time if the result is in the
		 * same order as the query was. All response elements
		 * before startptr are already processed */
		for (i = startptr; i < pairs->__size; i++)
		{
			if (!seen[i] && !strcmp(orig_items[itemptr],
					pairs->__ptr[i]->string1))
			{
				seen[i]++;
				break;
			}
		}

		if (i >= pairs->__size)
			/* No response for this query */
			continue;

		if (pairs->__ptr[i]->string2)
		{
			result[itemptr] = strdup(pairs->__ptr[i]->string2);
			if (!result[itemptr])
			{
				err_outofmemory(ctx);
				glite_freeStringArray(itemptr, result);
				free(seen);
				return NULL;
			}
		}
		else
			result[itemptr] = NULL;

		/* Update the startptr and skip as many elements as
		 * possible */
		if (i == startptr)
			while (startptr < pairs->__size && seen[startptr])
				startptr++;
	}

	free(seen);
	return result;
}

