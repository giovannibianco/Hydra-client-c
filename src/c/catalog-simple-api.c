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
 *  GLite Data Management - Simple Fireman API
 *
 *  Authors: 
 *      Gabor Gombas <Gabor.Gombas@cern.ch>
 *      Akos Frohner <Akos.Frohner@cern.ch>
 *  Version info: $Id: catalog-simple-api.c,v 1.2 2010-04-07 11:11:06 jwhite Exp $ 
 *  Release: $Name: not supported by cvs2svn $
 *
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/catalog/c/catalog-simple.h>

#include <cgsi_plugin.h>

#include <glite/data/glite-util.h>

#include <glite/data/catalog/metadata/c/metadataStub.h>
#include "internal.h"

/**********************************************************************
 * Local helper functions
 */

/* Convert the SOAP fault to an error message */
void _glite_catalog_fault_to_error(glite_catalog_ctx *ctx, const char *method)
{
	const char **code, **string, **detail;

	/* Reset the error class */
	ctx->errclass = GLITE_CATALOG_ERROR_NONE;

	/* Convert the error message to a fault */
	soap_set_fault(ctx->soap);

	/* Try to decode the exception that caused the fault */
	if (ctx->decode_exception && ctx->soap->fault)
	{
		/* Look for a SOAP 1.1 fault */
		if (ctx->soap->fault->detail)
			ctx->decode_exception(ctx, ctx->soap->fault->detail, method);
		/* Look for a SOAP 1.2 fault */
		if (ctx->soap->fault->SOAP_ENV__Detail)
			ctx->decode_exception(ctx,
				ctx->soap->fault->SOAP_ENV__Detail, method);
	}

	/* If we did not manage to decode the exception, try generic error
	 * decoding */
	if (ctx->errclass == GLITE_CATALOG_ERROR_NONE)
	{
		code = soap_faultcode(ctx->soap);
		string = soap_faultstring(ctx->soap);
		detail = soap_faultdetail(ctx->soap);

		/* If the SOAP 1.1 detail is empty, try the SOAP 1.2 detail */
		if (!detail && ctx->soap->fault &&
				ctx->soap->fault->SOAP_ENV__Detail)
			detail = (const char **)&ctx->soap->fault->SOAP_ENV__Detail->__any;

		/* Provide default messages */
		if (!code || !*code)
		{
			code = g_alloca(sizeof(*code));
			*code = "(SOAP fault code missing)";
		}
		if (!string || !*string)
		{
			string = g_alloca(sizeof(*string));
			*string = "(SOAP fault string missing)";
		}

		if (detail && *detail)
			glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_SOAP,
				"%s: SOAP fault: %s - %s (%s)", method, *code,
				*string, *detail);
		else
			glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_SOAP,
				"%s: SOAP fault: %s - %s", method, *code,
				*string);
	}

	soap_end(ctx->soap);
}

/* Check if this is a http:// URL */
static int is_http(const char *url)
{
	return url && !strncmp(url, HTTP_PREFIX, strlen(HTTP_PREFIX));
}

/* Check if this is a https:// URL */
static int is_https(const char *url)
{
	return url && !strncmp(url, HTTPS_PREFIX, strlen(HTTPS_PREFIX));
}

/* Check if this is a httpg:// URL */
static int is_httpg(const char *url)
{
	return url && !strncmp(url, HTTPG_PREFIX, strlen(HTTPG_PREFIX));
}

/**********************************************************************
 * Library interface functions
 */

glite_catalog_ctx *glite_catalog_new(const char *endpoint)
{
	glite_catalog_ctx *ctx;

	ctx = calloc(sizeof(*ctx), 1);
	if (!ctx)
		return NULL;

	if (endpoint)
	{
		ctx->endpoint = strdup(endpoint);
		if (!ctx->endpoint)
		{
			glite_catalog_free(ctx);
			return NULL;
		}
	}

	/* Initialize the SOAP descriptor */
	ctx->soap = soap_new();
	if (!ctx->soap)
	{
		glite_catalog_free(ctx);
		return NULL;
	}

	/* reset permissions */
	ctx->defaultUserPerm = GLITE_CATALOG_DEFAULT_USERPERM;
	ctx->defaultGroupPerm = GLITE_CATALOG_DEFAULT_GROUPPERM;
	ctx->defaultOtherPerm = GLITE_CATALOG_DEFAULT_OTHERPERM;

	/* set port type to none */
	ctx->port_type = GLITE_CATALOG_PORT_NONE;

	/* reset limits */
	ctx->readDir_limit = 0;
	ctx->query_limit = 0;
	ctx->locate_limit = 0;

	/* unknown functions at this point */
	ctx->decode_exception = NULL;

	/* this will trigger init_ctx */
	ctx->interface_version = NULL;

	return ctx;
}

void glite_catalog_free(glite_catalog_ctx *ctx)
{
	if (!ctx)
		return;

	free(ctx->interface_version);
	free(ctx->endpoint);
	free(ctx->last_error);
	if (ctx->soap)
	{
		soap_destroy(ctx->soap);
		soap_end(ctx->soap);
		free(ctx->soap);
	}
	free(ctx);
}

int _glite_catalog_init_endpoint(glite_catalog_ctx *ctx,
		struct Namespace *namespaces, const char *sd_type)
{
	int ret;

	if (!ctx)
		return -1;

	/* If we were given a direct URL, use it as it is. Otherwise, do a
	 * Service Discovery lookup */
	if (!is_http(ctx->endpoint) && !is_https(ctx->endpoint) && !is_httpg(ctx->endpoint))
	{
		char *error, *new_endpoint;

		new_endpoint = glite_discover_endpoint(sd_type, ctx->endpoint, &error);
		if (!new_endpoint)
		{
			glite_catalog_set_error(ctx,
				GLITE_CATALOG_ERROR_SERVICEDISCOVERY,
				"Service discovery: %s", error);
			free(error);
			return -1;
		}

		/* set the endpoint to this new one */
		free(ctx->endpoint);
		ctx->endpoint = new_endpoint;
	}

	/* Register the CGSI plugin if secure communication is requested */
	if (is_https(ctx->endpoint))
		ret = soap_cgsi_init(ctx->soap,
			CGSI_OPT_DISABLE_NAME_CHECK |
			CGSI_OPT_SSL_COMPATIBLE);
	else if (is_httpg(ctx->endpoint))
		ret = soap_cgsi_init(ctx->soap,
			CGSI_OPT_DISABLE_NAME_CHECK);
	else
		ret = 0;
	if (ret)
	{
		glite_catalog_set_error(ctx, GLITE_CATALOG_ERROR_SOAP,
			"Failed to initialize the GSI plugin");
		return -1;
	}

	/* Namespace setup should happen after CGSI plugin
	 * initialization */
	if (soap_set_namespaces(ctx->soap, namespaces))
	{
		_glite_catalog_fault_to_error(ctx, "Setting SOAP namespaces");
		return -1;
	}

	return 0;
}

const char *glite_catalog_get_endpoint(glite_catalog_ctx *ctx)
{
	if (!ctx)
		return NULL;

	return ctx->endpoint;
}

const char *glite_catalog_get_error(glite_catalog_ctx *ctx)
{
	/* This means we can still call glite_catalog_get_error() after
	 * a failed glite_catalog_new() and get a meaningful result */
	if (!ctx)
		return "Out of memory";

	if (!ctx->last_error)
		return "No error";
	return ctx->last_error;
}

glite_catalog_errclass glite_catalog_get_errclass(glite_catalog_ctx *ctx)
{
	if (!ctx)
		return GLITE_CATALOG_ERROR_UNKNOWN;
	return ctx->errclass;
}

void glite_catalog_set_verror(glite_catalog_ctx *ctx,
	glite_catalog_errclass errclass, const char *fmt, va_list ap)
{
	char buf[2048];

	/* Sanity check */
	if (!ctx)
		return;

	vsnprintf(buf, sizeof(buf), fmt, ap);

	free(ctx->last_error);
	ctx->last_error = strdup(buf);

	ctx->errclass = errclass;
}

void glite_catalog_set_error(glite_catalog_ctx *ctx,
	glite_catalog_errclass errclass, const char *fmt, ...)
{
	va_list ap;

	/* Sanity check */
	if (!ctx)
		return;

	va_start(ap, fmt);
	glite_catalog_set_verror(ctx, errclass, fmt, ap);
	va_end(ap);
}

void glite_catalog_set_default_Perm(glite_catalog_ctx *ctx,
	glite_catalog_Perm userPerm, glite_catalog_Perm groupPerm,
	glite_catalog_Perm otherPerm)
{
	if (!ctx)
		return;

	ctx->defaultUserPerm = userPerm;
	ctx->defaultGroupPerm = groupPerm;
	ctx->defaultOtherPerm = otherPerm;
}
