/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Catalog - Main function for the tools
 *
 *  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
 *  Version info: $Id: tool-main.h,v 1.1 2006-04-12 15:56:18 szamsu Exp $ 
 *  Release: $Name: not supported by cvs2svn $
 *
 */

#ifndef TOOL_MAIN_H
#define TOOL_MAIN_H

#include <glib.h>

/**********************************************************************
 * Constants
 */

/* Operation to perform for chmod, setacl, setdefacl and setdefperm */
typedef enum
{
	OP_DEL				= (1 << 0),
	OP_MODIFY			= (1 << 1),
	OP_SET				= (1 << 2)
} operation_code;

/* Sublect flags for chmod and setdefperm */
typedef enum
{
	USER				= (1 << 0),
	GROUP				= (1 << 1),
	OTHER				= (1 << 2)
} subject_code;

#define DEFAULT_BATCH_FACTOR		100


/**********************************************************************
 * Data type definitions
 */

typedef struct _acl_ctx			acl_ctx;
typedef struct _chmod_cmd		chmod_cmd;

struct _acl_ctx
{
	/* List of ACLs to add/modify */
	GList				*mod_acls;

	/* List of ACLs to delete */
	GList				*del_acls;

	/* List of ACLs to set */
	GList				*set_acls;
};

struct _chmod_cmd
{
	operation_code			opcode;
	subject_code			subject;
	glite_catalog_Perm		mask;
};


/**********************************************************************
 * Prototypes
 */

/* Parse tool-specific command line option */
int tool_parse_cmdline(int option, char *opt_arg);

/* Perform tool operation */
int tool_doit(glite_catalog_ctx *ctx, int argc, char *argv[]);

/**********************************************************************
 * Utility functions
 */

/* Print an informational message */
void info(const char *fmt, ...);

/* Print an error message */
void error(const char *fmt, ...);

/* Convert a glite_catalog_Perm to a string */
void print_perm(GString *dst, glite_catalog_Perm perm);

/* Convert a string to glite_catalog_Perm */
int parse_perm(const char *src, glite_catalog_Perm *perm);

/* Parse a chmod-style mode change specification */
GList *parse_modespec(const char *mode);

/* Apply a chmod-style mode change */
int apply_modes(glite_catalog_Permission *perm, GList *cmds);

/* Print the ACLs */
void print_acls(glite_catalog_Permission *perms);

/* Parse an ACL specification */
int parse_acl(acl_ctx *actx, char *opt_arg, operation_code op);

/* Read an ACL file */
int read_aclfile(acl_ctx *actx, const char *filename, operation_code op);

/* Perform ACL updates */
int update_acls(glite_catalog_ctx *ctx, acl_ctx *actx,
	glite_catalog_Permission *perm);

void acl_ctx_destroy(acl_ctx *actx);

/* Resolve a symlink recursively */
int resolve(glite_catalog_ctx *ctx, const char *symlink, char **lfn_out,
	glite_catalog_LFNStat **stat_out);

/**********************************************************************
 * Global variables exported by the tool
 */

/* Tool-specific command line options */
extern const char *tool_options;

/* Tool-specific help message */
extern const char *tool_help;

/* Tool-specific usage instructions */
extern const char *tool_usage;

/**********************************************************************
 * Global variables exported by the main module
 */

/* Verbosity level */
extern int verbose_flag;

/* Request quiet operation */
extern int quiet_flag;

#endif /* TOOL_MAIN_H */
