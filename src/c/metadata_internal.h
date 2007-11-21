#ifndef METADATA_INTERNAL_H
#define METADATA_INTERNAL_H


#include "internal.h"
#include <glite/data/catalog/metadata/c/metadataStub.h>

/**********************************************************************
 * SOAP type conversion functions
 */

int _glite_catalog_to_soap_StringPairArray(struct soap *soap,
	struct metadataArrayOf_USCOREtns1_USCOREStringPair *req,
	int nitems, const char *items[][2]);

char **_glite_catalog_flatten_soap_StringPairArray(glite_catalog_ctx *ctx, int nitems,
	const char * const orig_items[],
	struct metadataArrayOf_USCOREtns1_USCOREStringPair *pairs);

int _glite_catalog_to_soap_StringArray(struct soap *soap,
	struct metadataArrayOf_USCOREsoapenc_USCOREstring *req,
	int nitems, const char * const items[]);
char **_glite_catalog_from_soap_StringArray(glite_catalog_ctx *ctx,
	struct metadataArrayOf_USCOREsoapenc_USCOREstring *resp,
	int *resultCount);

#endif /* METADATA_INTERNAL_H */
