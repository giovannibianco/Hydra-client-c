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
 *  GLite Data Management - Simple Fireman API
 *
 *  Authors: 
 *      Gabor Gombas <Gabor.Gombas@cern.ch>
 *      Akos Frohner <Akos.Frohner@cern.c>
 *  Version info: $Id: metadata-simple.h,v 1.2 2010-04-07 11:11:06 jwhite Exp $
 *  Release: $Name: not supported by cvs2svn $
 *
 */

#ifndef GLITE_DATA_CATALOG_METADATA_SIMPLE_H
#define GLITE_DATA_CATALOG_METADATA_SIMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <time.h>

#include <glite/data/catalog/c/catalog-simple.h>

/**********************************************************************
 * Library interface functions
 */

int glite_metadata_get_query_limit(glite_catalog_ctx *ctx);

/**********************************************************************
 * Function prototypes - org.glite.data.catalog.service.ServiceBase
 */

/* Get the version of the remote endpoint.
 * The returned string must be freed by the caller. In case of an error,
 * NULL is returned. */
char *glite_metadata_getVersion(glite_catalog_ctx *ctx);

/* Get the schema version of the remote endpoint.
 * The returned string must be freed by the caller. In case of an error,
 * NULL is returned. */
char *glite_metadata_getSchemaVersion(glite_catalog_ctx *ctx);

/* Get the interface version of the remote endpoint.
 * The returned string must be freed by the caller. In case of an error,
 * NULL is returned. */
char *glite_metadata_getInterfaceVersion(glite_catalog_ctx *ctx);

/* Get the service metadata for a given key. The key must not be NULL.
 * The returned string must be freed by the caller. In case of an error,
 * NULL is returned. */
char *glite_metadata_getServiceMetadata(glite_catalog_ctx *ctx, const char *key);

/**********************************************************************
 * Function prototypes - org.glite.data.catalog.service.FASBase
 */

/* Set the permissions for an item */
int glite_metadata_setPermission(glite_catalog_ctx *ctx, const char *item,
	const glite_catalog_Permission *permission);

/* Set the permissions for multiple items simultaneously */
int glite_metadata_setPermission_multi(glite_catalog_ctx *ctx, int nitems,
	const char * const items[],
	const glite_catalog_Permission * const permissions[]);

/* Get the permissions of an item */
glite_catalog_Permission *glite_metadata_getPermission(glite_catalog_ctx *ctx,
	const char *item);

/* Get the permissions of multiple items. The result might contain NULL
 * pointers if there were no response for that particular item */
glite_catalog_Permission **glite_metadata_getPermission_multi(glite_catalog_ctx *ctx,
	int nitems, const char * const items[]);

/* Check if we have all the requested permissions for an item */
int glite_metadata_checkPermission(glite_catalog_ctx *ctx, const char *item,
	glite_catalog_Perm perm);

/* Check if we have all the requested permissions for all items */
int glite_metadata_checkPermission_multi(glite_catalog_ctx *ctx, int nitems,
	const char * const items[], glite_catalog_Perm perm);

/**********************************************************************
 * Function prototypes - org.glite.data.catalog.service.meta.MetadataBase
 */

/* Set the attribute values for an item */
int glite_metadata_setAttributes(glite_catalog_ctx *ctx, const char *item,
	int nattributes, const glite_catalog_Attribute * const attributes[]);

/* Clear the attribute values for an item */
int glite_metadata_clearAttributes(glite_catalog_ctx *ctx, const char *item,
	int nattributes, const char * const attributes[]);

/* Get the attribute values for an item. The result might contain NULL
 * pointers if there was no response for that particular item */
glite_catalog_Attribute **glite_metadata_getAttributes(glite_catalog_ctx *ctx,
	const char *item, int nattributes, const char * const attributes[],
	int *resultCount);

/* List all attributes of an item. The result might contain NULL
 * pointers if there was no response for that particular item */
glite_catalog_Attribute **glite_metadata_listAttributes(glite_catalog_ctx *ctx,
	const char *item, int *resultCount);

/* Perform a query on the catalog, retrieving the corresponding items. The 
 * result might contain NULL pointers if there was no response for that 
 * particular item */
char **glite_metadata_query(glite_catalog_ctx *ctx, const char *query,
	const char *type, int limit, int offset, int *resultCount);

/**********************************************************************
 * Function prototypes - org.glite.data.catalog.service.meta.MetadataCatalog
 */

/* Create an entry */
int glite_metadata_createEntry(glite_catalog_ctx *ctx, const char *item,
	const char *schema);

/* Create multiple entries */
int glite_metadata_createEntry_multi(glite_catalog_ctx *ctx, int nitems,
	const char **items[2]);

/* Remove an entry */
int glite_metadata_removeEntry(glite_catalog_ctx *ctx, const char *item);

/* Remove multiple entries */
int glite_metadata_removeEntry_multi(glite_catalog_ctx *ctx,
	int nitems, const char * const items[]);

/**********************************************************************
 * Function prototypes - org.glite.data.catalog.service.meta.MetadataSchema
 */

/* Describe a schema */
glite_catalog_Attribute **glite_metadata_describeSchema(glite_catalog_ctx *ctx,
	const char *schema, int *resultCount);

/* Get the list of schemas*/
char **glite_metadata_listSchemas(glite_catalog_ctx *ctx,
	int *resultCount);

#ifdef __cplusplus
}
#endif

#endif /* GLITE_DATA_CATALOG_METADATA_SIMPLE_H */

