/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Catalog - Main function for the tools
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *           Ricardo Rocha <Ricardo.Rocha@cern.ch>
 *  Version info: $Id: meta-tool-main.c,v 1.3 2007-11-30 17:47:15 szamsu Exp $ 
 *  Release: $Name: not supported by cvs2svn $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glite/data/hydra/c/eds-simple.h>
#include <glite/data/catalog/metadata/c/metadata-simple.h>

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tool-main.h"


/**********************************************************************
 * Global variables
 */

/* Location of the Metadata service */
static char *service_location;

/* List of Metadata Catalog context handles */
static glite_catalog_ctx * _ctx[1];
static glite_catalog_ctx_list ctx_list = { .count=0, .ctx=_ctx };

/* Command line options common to all the tools */
static const char *common_options = "hqs:vV";

/* Description of the common options */
static const char *common_help =
	"\t-h\t\tPrint this help text and exit.\n"
	"\t-q\t\tQuiet operation.\n"
	"\t-s URL\t\tUse the service at the specified URL\n"
	"\t-v\t\tBe more verbose.\n"
	"\t-V\t\tPrint the version number and exit.\n";

/* Flag to turn on verbose output */
int verbose_flag;

/* Flag to request quiet operation */
int quiet_flag;


/**********************************************************************
 * Logging functions
 */

void info(const char *fmt, ...)
{
	va_list ap;

	if (!verbose_flag)
		return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("\n");
	fflush(stdout);
	va_end(ap);
}

void error(const char *fmt, ...)
{
	char *p, *msg;
	va_list ap;

	if (quiet_flag)
		return;

	va_start(ap, fmt);
	msg = g_strdup_vprintf(fmt, ap);
	va_end(ap);

	/* If the command name is the same as the name of the failed method,
	 * we may get duplication at the beginning of the message, like
	 * mv: mv: Destination entry: "/foo" already exists!
	 * Remove the duplication. */
	p = strchr(msg, ':');
	if (p)
	{
		int len = p - msg + 1;
		if (msg[len + 1] && !strncmp(msg, msg + len + 1, len))
			memmove(msg, msg + len + 1, strlen(msg) - len);
	}
	fprintf(stderr, "ERROR %s\n", msg);
	fflush(stderr);
	g_free(msg);
}

/**********************************************************************
 * Main routine for the GLite catalog tools
 */

int main(int argc, char *argv[])
{
	char *options, *prog_name;
	int optlen, ret, c, i;

	/* Determine the program name we were called as */
	prog_name = strrchr(argv[0], '/');
	if (!prog_name)
		prog_name = argv[0];
	else
		prog_name++;

	/* Build the option string for getopt() */
	optlen = strlen(common_options) + 1;
	if (tool_options)
		optlen += strlen(tool_options);

	options = malloc(optlen);
	if (!options)
	{
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	strcpy(options, common_options);
	if (tool_options)
		strcat(options, tool_options);

	/* Parse the command line options */
	while ((c = getopt(argc, argv, options)) != -1) switch (c)
	{
		case 'h':
			printf("Usage: %s [options]", prog_name);
			if (tool_usage)
				printf(" %s\n", tool_usage);
			else
				printf("\n");

			printf("Available options:\n%s", common_help);
			if (tool_help)
				printf("%s", tool_help);
			exit(EXIT_SUCCESS);
		case 'q':
			quiet_flag++;
			verbose_flag = 0;
			break;
		case 's':
			service_location = optarg;
			break;
		case 'v':
			verbose_flag++;
			quiet_flag = 0;
			break;
		case 'V':
			printf("%s\n", PACKAGE_VERSION);
			exit(EXIT_SUCCESS);
		case ':':
			fprintf(stderr, "Option argument is missing\n");
			exit(EXIT_FAILURE);
		case '?':
			fprintf(stderr, "Unknown command line option\n");
			exit(EXIT_FAILURE);
		default:
			ret = tool_parse_cmdline(c, optarg);
			if (ret)
			{
				fprintf(stderr, "Illegal command line arguments\n");
				exit(EXIT_FAILURE);
			}
			break;
	}

	/* Drop the processed options from the argument list */
	free(options);
	argc -= optind;
	argv += optind;

	if (service_location) 
	{
		ctx_list.count = 1;
		ctx_list.ctx[0] = glite_catalog_new(service_location);
		if(!ctx_list.ctx[0]) {
			error("Failed to allocate the Metadata Catalog context: %s",
				service_location);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		/* Find endpoint for each of default services */
        	char **endpoints;
		char *errstr;

		endpoints = glite_eds_get_catalog_endpoints(&ctx_list.count, &errstr);
		if (!endpoints) 
		{
			fprintf(stderr, errstr);
			exit(EXIT_FAILURE);
		}

		ctx_list.ctx = malloc(sizeof(glite_catalog_ctx *) * ctx_list.count);
		if (!ctx_list.ctx)
		{
			error("out of memory");
			exit(EXIT_FAILURE);
		}

		/* Allocate a Metadata Catalog context for each service */
		for(i = 0; i < ctx_list.count; i++) 
		{
			ctx_list.ctx[i] = glite_catalog_new(endpoints[i]);
			if (!ctx_list.ctx[i])
			{
				error("Failed to allocate the Metadata Catalog context: %s",
					endpoints[i]);
				exit(EXIT_FAILURE);
			}
		}

		// free_list(endpoints, ctx_list.count); 
		for(i = 0; i < ctx_list.count; i++)
			free(endpoints[i]);
		free(endpoints);
	}


	/* Write some information about services */
	if (verbose_flag)
	{
		for(i = 0; i < ctx_list.count; i++) 
		{
			if (print_service_info(ctx_list.ctx[i]))
				exit(EXIT_FAILURE);
		}
	}

	/* Perform the operation */
	ret = tool_doit(&ctx_list, argc, argv);

	/* Cleanup */
	for(i = 0; i < ctx_list.count; i++)
		glite_catalog_free(ctx_list.ctx[i]);

	return ret;
}
