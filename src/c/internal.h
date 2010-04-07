/** 
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
 */

#ifndef SIMPLE_API_INTERNAL_H
#define SIMPLE_API_INTERNAL_H


#include <glib.h>

/**********************************************************************
 * Constants
 */

#define HTTP_PREFIX			"http://"
#define HTTPS_PREFIX			"https://"
#define HTTPG_PREFIX			"httpg://"

typedef enum 
{
	GLITE_CATALOG_PORT_NONE,
	GLITE_CATALOG_PORT_FIREMAN,
	GLITE_CATALOG_PORT_SEINDEX,
	GLITE_CATALOG_PORT_METADATA
} _glite_catalog_port_type;


/**********************************************************************
 * Data type declarations
 */

/* gLite Catalog datatype declarations for the interface */
/* grep '^struct glite__' <glite/data/catalog/metadata/c/metadataStub.h> */

struct glite__ACLEntry;
struct glite__Attribute;
struct glite__AuthorizationException;
struct glite__BasicPermission;
struct glite__CatalogException;
struct glite__ExistsException;
struct glite__InternalException;
struct glite__InvalidArgumentException;
struct glite__NotExistsException;
struct glite__Perm;
struct glite__Permission;
struct glite__PermissionEntry;
struct glite__StringPair;

/**********************************************************************
 * Data type definitions
 */

/* interface specific exception decoder function type */
typedef void (*glite_catalog_decode_exception_func)(glite_catalog_ctx *ctx,
            struct SOAP_ENV__Detail *detail, const char *method);

struct _glite_catalog_ctx
{
	struct soap			*soap;
	char				*endpoint;
	char				*last_error;
	glite_catalog_errclass		errclass;
	_glite_catalog_port_type	port_type;

	glite_catalog_Perm		defaultUserPerm;
	glite_catalog_Perm		defaultGroupPerm;
	glite_catalog_Perm		defaultOtherPerm;

	/* virtual methods ... */
	glite_catalog_decode_exception_func	decode_exception;

	/* Data specific to an endpoint */
	char				*interface_version;
	int				readDir_limit;
	int				query_limit;
	int				locate_limit;
};


/**********************************************************************
 * Function prototypes - library management functions
 */

/*
 * Initialize the SOAP structure of the context, calling service
 * discovery, if necessary. The additional parameters are the 
 * namespaces for the SOAP context and the service type string 
 * for the optional service discovery.
 */
int _glite_catalog_init_endpoint(glite_catalog_ctx *ctx, 
    struct Namespace *namespaces, const char *sd_type);

/* Convert the SOAP fault to an error message */
void _glite_catalog_fault_to_error(glite_catalog_ctx *ctx, const char *method);

/**********************************************************************
 * SOAP type conversion functions
 */

struct glite__Perm *_glite_catalog_to_soap_Perm(struct soap *soap, glite_catalog_Perm perm);
struct glite__Permission *_glite_catalog__glite_catalog_to_soap_Permission(struct soap *soap,
	const glite_catalog_Permission *permission);
glite_catalog_Permission *_glite_catalog_from_soap_Permission(glite_catalog_ctx *ctx,
	const struct glite__Permission *spermission);
struct glite__Attribute *_glite_catalog_to_soap_Attribute(struct soap *soap,
	const glite_catalog_Attribute *attr);
glite_catalog_Attribute *_glite_catalog_from_soap_Attribute(glite_catalog_ctx *ctx,
	const struct glite__Attribute *sattr);

/**********************************************************************
 * Inline helper functions
 */

static inline int64_t to_soap_time(const struct timespec *ts)
{
	return (int64_t)ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
}

static inline void err_outofmemory(glite_catalog_ctx *ctx)
{
	glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_OUTOFMEMORY,
		"Out of memory");
}

static inline void err_invarg(glite_catalog_ctx *ctx, const char *msg)
{
	glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_INVALIDARGUMENT,
		"%s", msg);
}

static inline void err_exists(glite_catalog_ctx *ctx, const char *msg)
{
	glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_EXISTS,
		"%s", msg);
}

static inline void err_notexists(glite_catalog_ctx *ctx, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	glite_catalog_set_verror(ctx, GLITE_CATALOG_ERROR_NOTEXISTS,
		msg, ap);
	va_end(ap);
}

static inline void err_soap(glite_catalog_ctx *ctx, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	glite_catalog_set_verror(ctx, GLITE_CATALOG_ERROR_SOAP,
		msg, ap);
	va_end(ap);
}

#endif /* SIMPLE_API_INTERNAL_H */
