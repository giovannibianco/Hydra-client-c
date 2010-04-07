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
 *  gLite Data Management - Simple Catalog API
 *
 *  Authors: 
 *      Gabor Gombas <Gabor.Gombas@cern.ch>
 *      Akos Frohner <Akos.Frohner@cern.ch>
 *  Version info: $Id: catalog-simple.h,v 1.2 2010-04-07 11:11:06 jwhite Exp $
 *  Release: $Name: not supported by cvs2svn $
 *
 */

#ifndef GLITE_DATA_CATALOG_SIMPLE_H
#define GLITE_DATA_CATALOG_SIMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <time.h>

/**********************************************************************
 * Data type declarations
 */

/*
 * Opaque data structure used by the library.
 */
typedef struct _glite_catalog_ctx		glite_catalog_ctx;

/* Fireman data structures */
typedef struct _glite_catalog_ACLEntry		glite_catalog_ACLEntry;
typedef struct _glite_catalog_Attribute		glite_catalog_Attribute;
typedef struct _glite_catalog_FCEntry		glite_catalog_FCEntry;
typedef struct _glite_catalog_FRCEntry		glite_catalog_FRCEntry;
typedef struct _glite_catalog_GUIDStat		glite_catalog_GUIDStat;
typedef struct _glite_catalog_LFNStat		glite_catalog_LFNStat;
typedef struct _glite_catalog_Permission	glite_catalog_Permission;
typedef struct _glite_catalog_RCEntry		glite_catalog_RCEntry;
typedef struct _glite_catalog_SURLEntry		glite_catalog_SURLEntry;
typedef struct _glite_catalog_Stat		    glite_catalog_Stat;

typedef char *(*glite_catalog_StringPairArray)[2];


/**********************************************************************
 * Constants
 */

/**
 * \brief Error categories. These are analogous to the exception classes in Java.
 */
typedef enum
{
	GLITE_CATALOG_EXCEPTION_CATALOG = -6,
	GLITE_CATALOG_EXCEPTION_INTERNAL = -5,
	GLITE_CATALOG_EXCEPTION_AUTHORIZATION = -4,
	GLITE_CATALOG_EXCEPTION_NOTEXISTS = -3,
	GLITE_CATALOG_EXCEPTION_INVALIDARGUMENT = -2,
	GLITE_CATALOG_EXCEPTION_EXISTS = -1,
	GLITE_CATALOG_ERROR_NONE,
	GLITE_CATALOG_ERROR_UNKNOWN,
	GLITE_CATALOG_ERROR_INVALIDARGUMENT,
	GLITE_CATALOG_ERROR_OUTOFMEMORY,
	GLITE_CATALOG_ERROR_SERVICEDISCOVERY,
	GLITE_CATALOG_ERROR_NOTEXISTS,
	GLITE_CATALOG_ERROR_EXISTS,
	GLITE_CATALOG_ERROR_SOAP
} glite_catalog_errclass;

/**
 * \brief Permission bits
 */
typedef enum
{
	GLITE_CATALOG_PERM_PERMISSION	= (1 << 0),
	GLITE_CATALOG_PERM_REMOVE	= (1 << 1),
	GLITE_CATALOG_PERM_READ		= (1 << 2),
	GLITE_CATALOG_PERM_WRITE	= (1 << 3),
	GLITE_CATALOG_PERM_LIST		= (1 << 4),
	GLITE_CATALOG_PERM_EXECUTE	= (1 << 5),
	GLITE_CATALOG_PERM_GETMETADATA	= (1 << 6),
	GLITE_CATALOG_PERM_SETMETADATA	= (1 << 7)
} glite_catalog_Perm;

/**
 * \brief Total number of permission bits.
 */
#define GLITE_CATALOG_PERM_NUMBITS	8

/**
 * \brief Default user permissions for newly created objects
 */
#define GLITE_CATALOG_DEFAULT_USERPERM	\
	(\
		GLITE_CATALOG_PERM_PERMISSION | \
		GLITE_CATALOG_PERM_REMOVE | \
		GLITE_CATALOG_PERM_READ | \
		GLITE_CATALOG_PERM_WRITE | \
		GLITE_CATALOG_PERM_LIST | \
		GLITE_CATALOG_PERM_EXECUTE | \
		GLITE_CATALOG_PERM_GETMETADATA | \
		GLITE_CATALOG_PERM_SETMETADATA \
	 )

/**
 * \brief Default group permissions for newly created objects
 */
#define GLITE_CATALOG_DEFAULT_GROUPPERM \
	(\
		GLITE_CATALOG_PERM_READ | \
		GLITE_CATALOG_PERM_LIST \
	 )

/* Default other permissions for newly created objects */
#define GLITE_CATALOG_DEFAULT_OTHERPERM (0)

/**
 * \brief Directory operation flags.
 */
typedef enum
{
	GLITE_CATALOG_CREATE_PARENTS	= (1 << 0),
	GLITE_CATALOG_COPYPERM		= (1 << 1)
} glite_catalog_DirOpFlag;

/**
 * \brief Directory entry types.
 */
typedef enum
{
	GLITE_CATALOG_FTYPE_UNKNOWN = -1,
	GLITE_CATALOG_FTYPE_DIRECTORY,
	GLITE_CATALOG_FTYPE_FILE,
	GLITE_CATALOG_FTYPE_SYMLINK,
	GLITE_CATALOG_FTYPE_VIRTUALDIR
} glite_catalog_Filetype;

/**
 * \brief Directory tree traversal flags.
 */
typedef enum
{
	/**
   * Fill the permission field of the FCEntry objects passed to the
	 * callback function.
   */
	GLITE_CATALOG_EXP_WITHPERM	= (1 << 0),
	/**
   * Fill the guid field of the FCEntry objects passed to the callback
	 * function.
   */
	GLITE_CATALOG_EXP_WITHGUID	= (1 << 1),
	/**
   * Traverse directories recursively.
   */
	GLITE_CATALOG_EXP_RECURSIVE	= (1 << 2),
	/**
   * Report non-existant leaf nodes instead of failing with an error.
   */
	GLITE_CATALOG_EXP_NOTEXIST_OK	= (1 << 3),
	/**
   * If the argument is a directory, report its contents instead of the
	 * directory itself, but do not recurse into subdirectories.
   */
	GLITE_CATALOG_EXP_DIRECTORY	= (1 << 4)
} glite_catalog_exp_flag;

/**
 * \brief Traversal direction.
 */
typedef enum
{
	/**
   * Used for non-directories and for directories before reporting
	 * their contents
   */
	GLITE_CATALOG_EXP_PRE,
	/**
   * Used for directories after their contents were reported
   */
	GLITE_CATALOG_EXP_POST
} glite_catalog_exp_dir;

/**
 * \defgroup catalog_data_types Catalog Data Types
 * @{
 */

/**
 * An ACL entry.
 */
struct _glite_catalog_ACLEntry
{
	char				*principal; /**< The name of the principal. */
	glite_catalog_Perm		principalPerm; /**< The corresponding permissions. */
};

/**
 * The general permission-structure of an object, with the\
 * default POSIX-like permissions and the ACLs.
 */
struct _glite_catalog_Permission
{
	char				*userName; /**< The username of the owner */
	char				*groupName; /**< The name of the owner group */

	glite_catalog_Perm		userPerm; /**< Permissions related to the owner. */
	glite_catalog_Perm		groupPerm; /**< Permissions related to the group. */
	glite_catalog_Perm		otherPerm; /**< Permissions related to the world - everyone else. */

	unsigned int			acl_cnt; /**< Number of the corresponding ACL entries. */
	glite_catalog_ACLEntry		**acl; /**< Array of the corresponding ACL entries. */
};

/**
 * Catalog entry status.
 */
struct _glite_catalog_Stat
{
	struct timespec			modifyTime; /**< Time of last modification. */
	struct timespec			creationTime; /**< Time of creation. */
	uint64_t			size; /**< Size of the file. */
};

/**
 * LFN status.
 */
struct _glite_catalog_LFNStat
{
	struct timespec			modifyTime; /**< Time of last modification. */
	struct timespec			creationTime; /**< Time of creation. */
	uint64_t			size; /**< Size of the file. */
 /**
  * \brief Type of the file.
  *
  * Possible values:
  * 0 - directory.
  * 1 - file.
  * 2 - symlink.
  * 3 - virtual directory.
  */
	glite_catalog_Filetype		type;
	char				*data; /**< If the LFN is a symlink, the data field has the name of the pointed LFN. */
	uint64_t			validityTime;	/**< Offset from creationTime, in milliseconds. 0 means infinity. */
};

/**
 * The GUID stats struct.
 */
struct _glite_catalog_GUIDStat
{
	struct timespec			modifyTime; /**< Time of last modification. */
	struct timespec			creationTime; /**< Time of creation. */
	uint64_t			size; /**< Size of the file. */

	char				*checksum; /**< The checksum string. */
 /**
  * A status integer. Reserved for future usage.
  */
	int32_t				status;
};

/**
 * The File Catalog Entry struct.
 */
struct _glite_catalog_FCEntry
{
	char				*lfn; /**< The Logical FileName. */
	char				*guid; /**< The unique ID of the file. */
	glite_catalog_Permission	*permission; /**< The corresponding permission-structure. */
	glite_catalog_LFNStat		lfnStat; /**< Status informations. */
};

/**
 * A basic name/value pair with an associated type.
 */
struct _glite_catalog_Attribute
{
	char				*name; /**< The name of the attribute. */
	char				*value; /**< The value of the attribute. */
	char				*type; /**< The name of the type. */
};

/**
 * The Replica Catalog Entry struct.
 */
struct _glite_catalog_RCEntry
{
	char				*guid; /**< The unique ID of the file we are replicating. */
	glite_catalog_Permission	*permission; /**< The permissions. */
	glite_catalog_GUIDStat		guidStat; /**< Status informations. */
	unsigned int			surlStats_cnt; /**< Number of the associated SURLs. */
	glite_catalog_SURLEntry		**surlStats; /**< Array of the SURLs. */
};

/**
 * \brief The File Replica Catalog Entry. A so-to-say substruct of glite_catalog_RCEntry,
 * it contains information about the LFN, too.
 */
struct _glite_catalog_FRCEntry
{
	char				*lfn; /**< The Logical FileName. */
	char				*guid; /**< The unique ID of the file. */
	glite_catalog_Permission	*permission; /**< The permissions. */
	glite_catalog_LFNStat		lfnStat; /**< Stat of the LFN. */
	glite_catalog_GUIDStat		guidStat; /**< Stat of the GUID. */
	unsigned int			surlStats_cnt; /**< Number of the associated SURLs. */
	glite_catalog_SURLEntry		**surlStats; /**< Array of the SURLs. */
};

/**
 * \brief The Storage Uniform Resource Locator Entry.
 *
 * The SURL itself is part of its stat to simplify the RCEntry
 * interaction and not to duplicate data.
 */
struct _glite_catalog_SURLEntry
{
  /**
   * Wheter this is the master replica or not. Only one master
   * replica is allowed to exist for every GUID.
   */
	int				masterReplica; 
	struct timespec			creationTime; /**< Time of creation. */
	struct timespec			modifyTime; /**< Time of last modification. */
	char				*surl; /**< The SURL string. */
};

/**
 * @}
 */

/**********************************************************************
 * General guidelines:
 * - Functions that return a pointer return NULL when there is an error.
 * - Functions that return 'int' return 0 when successful and -1 in case
 *   of an error.
 * - Any objects returned by a function is owned by the caller and has to
 *   be deallocated by the caller.
 */

/**
 * \defgroup catalog_lib Catalog Management Functions
 * @{
 */

/* Service type used for Service Discovery */
#define GLITE_FIREMAN_SD_TYPE		"org.glite.FiremanCatalog"
#define GLITE_SEINDEX_SD_TYPE		"org.glite.SEIndex"
#define GLITE_METADATA_SD_TYPE		"org.glite.Metadata"

/* Environment variables to override default service types */
#define GLITE_FIREMAN_SD_ENV		"GLITE_SD_FIREMAN_TYPE"
#define GLITE_SEINDEX_SD_ENV		"GLITE_SD_SEINDEX_TYPE"
#define GLITE_METADATA_SD_ENV		"GLITE_SD_METADATA_TYPE"

/**
 * \brief Allocates a new catalog context. 
 *
 * The context can be used to access only one service. 
 * 
 * If the endpoint is not a URL (HTTP, HTTPS or HTTPG), then
 * service discovery is used to locate the real service endpoint.
 * In this case the 'endpoint' parameter is the input of the 
 * service discovery. 
 *
 * This mechanism is executed upon the first service specific usage 
 * of the context. After that point methods using the same context,
 * from any other interface will return an error.
 *
 * @param endpoint The name of the endpoint. Either an URL or a
 *  service name.
 * @return the context or NULL if memory allocation has failed.
 */
glite_catalog_ctx *glite_catalog_new(const char *endpoint);

/**
 * Destroys a fireman context.
 *
 * @param ctx The global context.
 */
void glite_catalog_free(glite_catalog_ctx *ctx);

/** 
 * Get the current endpoint.
 *
 * @param ctx The global context.
 * @return the current endpoint used by the fireman context.
 */
const char *glite_catalog_get_endpoint(glite_catalog_ctx *ctx);

/**
 * \brief Gets the error message for the last failed operation.
 *
 * The returned pointer is valid only till the next call to any of
 * the library's functions with the same context pointer.
 *
 * @param ctx The global context.
 * @return the error message.
 */
const char *glite_catalog_get_error(glite_catalog_ctx *ctx);

/**
 * \brief Determines the class of the last error.
 *
 * @param ctx The global context.
 * @return the proper glite_catalog_errclass.
 */
glite_catalog_errclass glite_catalog_get_errclass(glite_catalog_ctx *ctx);

/**
 * \brief Set the error message in the context.
 *
 * @param ctx The global context.
 * @param errclass The error class.
 * @param fmt The format of the error message.
 */
void glite_catalog_set_error(glite_catalog_ctx *ctx,
	glite_catalog_errclass errclass, const char *fmt, ...);

/**
 * \brief Set the error message in the context.
 * 
 * @param ctx The global context.
 * @param errclass The error class.
 * @param fmt The format of the error message.
 * @param ap The variable-length argument list. 
 */
void glite_catalog_set_verror(glite_catalog_ctx *ctx,
	glite_catalog_errclass errclass, const char *fmt, va_list ap);

/**
 * Sets the default permission for all future glite_catalog_Permission
 * objects.
 *
 * @param ctx The global context.
 * @param userPerm The permissions of the owner.
 * @param groupPerm The permissions of the owner-group.
 * @param otherPerm The permissions of everyone else.
 */
void glite_catalog_set_default_Perm(glite_catalog_ctx *ctx,
	glite_catalog_Perm userPerm, glite_catalog_Perm groupPerm,
	glite_catalog_Perm otherPerm);

/**
 * Utility function: free a char *[][2] array.
 *
 * @param nitems Number of items in the array.
 * @param array The array that is to be freed.  
 */
void glite_catalog_freeStringPairArray(int nitems, char *array[][2]);

/**
 * @}
 */

/**
 * \defgroup catalog_data_manipulation Fireman Data Management Functions
 * @{
 */


/**
 * Allocates a new ACL entry.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param principal The name of the ACL-Entry.
 * @param principalPerm The permissions of this ACL.
 *
 * @return The requested ACLEntry struct.
 */
glite_catalog_ACLEntry *glite_catalog_ACLEntry_new(glite_catalog_ctx *ctx,
	const char *principal, glite_catalog_Perm principalPerm);
/**
 * Destroys an ACL entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The ACLEntry that is to be freed.
 */
void glite_catalog_ACLEntry_free(glite_catalog_ctx *ctx,
	glite_catalog_ACLEntry *entry);
/**
 * Destroys a list of ACL entries.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems  Number of items in the array.
 * @param entries The array of entries that need to be freed.
 */
void glite_catalog_ACLEntry_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_ACLEntry *entries[]);
/**
 * Makes a copy of an ACL entry 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param acl The ACL that we want to clone.
 *
 * @return The cloned ACL.
 */
glite_catalog_ACLEntry *glite_catalog_ACLEntry_clone(glite_catalog_ctx *ctx,
	const glite_catalog_ACLEntry *acl);

/**
 * Allocates a new permission record.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 *
 * @return The new Permission struct.
 */
glite_catalog_Permission *glite_catalog_Permission_new(glite_catalog_ctx *ctx);
/**
 * Destroys a permission record.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param permission the permission struct that needs to be freed.
 */
void glite_catalog_Permission_free(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission);
/**
 * Destroys a list of permission records 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems  Number of items in the array.
 * @param permissions The array of permission records that need to be freed.
 */
void glite_catalog_Permission_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_Permission *permissions[]);
/**
 * Makes a copy of a permission record.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param permission The permission record you want to be cloned.
 *
 * @return The cloned permission record.
 */
glite_catalog_Permission *glite_catalog_Permission_clone(glite_catalog_ctx *ctx,
	const glite_catalog_Permission *permission);
/**
 * Appends an ACL entry to a permission record. The ACL entry is copied.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param permission The permission record to be appended.
 * @param aclEntry The ACL to add.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_Permission_addACLEntry(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission,
	const glite_catalog_ACLEntry *aclEntry);
/**
 * Removes the ACL entry for the given principal.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param permission The permission record that which we want to remove from.
 * @param principal The name of the ACL we want to remove.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_Permission_delACLEntry(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission, const char *principal);
/**
 * Sets the userName field.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param permission The permission record in which we want to set the userName.
 * @param userName The string to be used as userName.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_Permission_setUserName(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission, const char *userName);
/**
 * Sets the groupName field.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param permission The permission record in which we want to set the userName. 
 * @param groupName The string to be used as groupName. 
 *
 * @return 0 when successful and -1 in case of an error. 
 */
int glite_catalog_Permission_setGroupName(glite_catalog_ctx *ctx,
	glite_catalog_Permission *permission, const char *groupName);

/**
 * Allocates a new file catalog entry.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param lfn The Logical FileName.
 *
 * @return The new FileCatalog Entry.
 */
glite_catalog_FCEntry *glite_catalog_FCEntry_new(glite_catalog_ctx *ctx,
	const char *lfn);
/**
 * Destroys a file catalog entry.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The entry to be disposed.
 */
void glite_catalog_FCEntry_free(glite_catalog_ctx *ctx,
	glite_catalog_FCEntry *entry);
/**
 * Destroys an array of file catalog entries.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of items in the array.
 * @param entries The array of entries to be disposed.
 */
void glite_catalog_FCEntry_freeArray(glite_catalog_ctx *ctx,
	int nitems, glite_catalog_FCEntry *entries[]);
/**
 * Clones a file catalog entry.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The File Catalog Entry to be cloned.
 *
 * @return The cloned entry.
 */
glite_catalog_FCEntry *glite_catalog_FCEntry_clone(glite_catalog_ctx *ctx,
	const glite_catalog_FCEntry *entry);
/**
 * Sets the GUID for a file catalog entry.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The FC entry that needs to be changed.
 * @param guid The new value of the GUID field.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_FCEntry_setGuid(glite_catalog_ctx *ctx,
	glite_catalog_FCEntry *entry, const char *guid);
/**
 * Update the stat, guid and permissions from the catalog.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The updating FC entry.
 * @param withPermissions Whether you want to update the permissions.
 * @param withGuid Whether you want to update the GUID.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_FCEntry_update(glite_catalog_ctx *ctx,
	glite_catalog_FCEntry *entry, int withPermissions, int withGuid);

/**
 * Allocates a new attribute.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param name The name of the new attribute.
 * @param value The value of the new attribute.
 * @param type The type of the new attribute.
 *
 * @return The new attribute structure.
 */
glite_catalog_Attribute *glite_catalog_Attribute_new(glite_catalog_ctx *ctx,
	const char *name, const char *value, const char *type);
/**
 * Destroys an attribute.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param attr The attribute to be destroyed.
 */
void glite_catalog_Attribute_free(glite_catalog_ctx *ctx,
	glite_catalog_Attribute *attr);
/**
 * Destroys an array of attributes.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of the items in the array.
 * @param attr The array of attributes to be disposed.
 */
void glite_catalog_Attribute_freeArray(glite_catalog_ctx *ctx,
	int nitems, glite_catalog_Attribute *attr[]);
/**
 * Clones an attribute.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param orig The attribute to be cloned.
 *
 * @return The cloned attribute.
 */
glite_catalog_Attribute *glite_catalog_Attribute_clone(glite_catalog_ctx *ctx,
	glite_catalog_Attribute *orig);

/**
 * Allocates a new replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param guid The GUID of the new entry.
 *
 * @return The new ReplicaCatalog entry.
 */
glite_catalog_RCEntry *glite_catalog_RCEntry_new(glite_catalog_ctx *ctx,
	const char *guid);
/**
 * Destroys a replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The entry to be destroyed.
 */
void glite_catalog_RCEntry_free(glite_catalog_ctx *ctx,
	glite_catalog_RCEntry *entry);
/**
 * Destroys a list of replica catalog entries. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of items in the array.
 * @param entries Array of RC entries to be disposed.
 */
void glite_catalog_RCEntry_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_RCEntry *entries[]);
/**
 * Clones a new replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param orig the RC entry to be cloned.
 *
 * @return The cloned RC entry.
 */
glite_catalog_RCEntry *glite_catalog_RCEntry_clone(glite_catalog_ctx *ctx,
	glite_catalog_RCEntry *orig);
/**
 * Adds a replica URL to a replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The existing RC entry which the new URL is added to.
 * @param surl The SURL to be added.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_RCEntry_addSurl(glite_catalog_ctx *ctx,
	glite_catalog_RCEntry *entry, const glite_catalog_SURLEntry *surl);
/**
 * Sets the checksum of a replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The existing RC entry which the new URL is added to.
 * @param checksum The checksum string to be inserted. 
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_RCEntry_setChecksum(glite_catalog_ctx *ctx,
	glite_catalog_RCEntry *entry, const char *checksum);

/**
 * Allocates a new replica entry.
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param surl The SURL of the new replica entry.
 * @param isMaster Wheter this is the master replica.
 *
 * @return The new SURLEntry stucture.
 */
glite_catalog_SURLEntry *glite_catalog_SURLEntry_new(glite_catalog_ctx *ctx,
	const char *surl, int isMaster);
/**
 * Destroys a replica entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The SURLEntry to be disposed.
 */
void glite_catalog_SURLEntry_free(glite_catalog_ctx *ctx,
	glite_catalog_SURLEntry *entry);
/**
 * Destroys a list of replica entries. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of items in the array.
 * @param entries Array of SURLEntries to be disposed.
 */
void glite_catalog_SURLEntry_freeArray(glite_catalog_ctx *ctx,
	int nitems, glite_catalog_SURLEntry *entries[]);
/**
 * Copies a replica entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The SURLEntry to be cloned.
 *
 * @return The cloned entry.
 */
glite_catalog_SURLEntry *glite_catalog_SURLEntry_clone(glite_catalog_ctx *ctx,
	const glite_catalog_SURLEntry *entry);

/**
 * Allocates a new stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 *
 * @return The new stat structure.
 */
glite_catalog_Stat *glite_catalog_Stat_new(glite_catalog_ctx *ctx);
/**
 * Destroys a stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param stat the structure to be disposed.
 */
void glite_catalog_Stat_free(glite_catalog_ctx *ctx,
	glite_catalog_Stat *stat);
/**
 * Destroys an array of stat structures. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of items in the array.
 * @param stats The array of stat structures to be freed.
 */
void glite_catalog_Stat_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_Stat *stats[]);
/**
 * Clones a stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param orig The structure that is to be cloned.
 *
 * @return The cloned stat structure.
 */
glite_catalog_Stat *glite_catalog_Stat_clone(glite_catalog_ctx *ctx,
	glite_catalog_Stat *orig);

/**
 * Allocates a new LFN stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 *
 * @return The new LFNStat structure.
 */
glite_catalog_LFNStat *glite_catalog_LFNStat_new(glite_catalog_ctx *ctx);
/**
 * Destroys an LFNStat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param stat The stucture to be disposed.
 */
void glite_catalog_LFNStat_free(glite_catalog_ctx *ctx,
	glite_catalog_LFNStat *stat);
/**
 * Destroys an array of LFN stat structures. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of items in the array.
 * @param stats Array of LFNStats to be disposed.
 */
void glite_catalog_LFNStat_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_LFNStat *stats[]);
/**
 * Clones an LFN stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param orig The LFNStat structure to be cloned.
 *
 * @return The cloned structure.
 */
glite_catalog_LFNStat *glite_catalog_LFNStat_clone(glite_catalog_ctx *ctx,
	glite_catalog_LFNStat *orig);
/**
 * Copies an LFN stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param dest The destination LFNStat structure. Must already exist.
 * @param src The source LFNStat structure, from which we copy the data.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_LFNStat_copy(glite_catalog_ctx *ctx,
	glite_catalog_LFNStat *dest, const glite_catalog_LFNStat *src);

/**
 * Allocates a new GUIDStat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 *
 * @return The new GUIDStat structure.
 */
glite_catalog_GUIDStat *glite_catalog_GUIDStat_new(glite_catalog_ctx *ctx);
/**
 * Destroys a GUID stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param stat The GUIDStat structure to be disposed.
 */
void glite_catalog_GUIDStat_free(glite_catalog_ctx *ctx,
	glite_catalog_GUIDStat *stat);
/**
 * Destroys an array of GUID stat structures. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of items in the array.
 * @param stats The array of GUIDStats to be disposed.
 */
void glite_catalog_GUIDStat_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_GUIDStat *stats[]);
/**
 * Clones a GUID stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param orig The GUIDStat structure to be cloned.
 *
 * @return The cloned GUIDStat structure.
 */
glite_catalog_GUIDStat *glite_catalog_GUIDStat_clone(glite_catalog_ctx *ctx,
	glite_catalog_GUIDStat *orig);
/**
 * Copies a GUID stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param dest The destination GUIDStat structure. Must already exist.
 * @param src The source structure to copy from.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_GUIDStat_copy(glite_catalog_ctx *ctx,
	glite_catalog_GUIDStat *dest, const glite_catalog_GUIDStat *src);
/**
 * Sets the checksum of a GUID stat structure. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param stat The GUIDStat structure to be modified.
 * @param checksum The new checksum value.
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_GUIDStat_setChecksum(glite_catalog_ctx *ctx,
	glite_catalog_GUIDStat *stat, const char *checksum);

/**
 * Allocates a new file and replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param lfn The Logical FileName of the entry.
 *
 * @return The new File and ReplicaCatalog entry.
 */
glite_catalog_FRCEntry *glite_catalog_FRCEntry_new(glite_catalog_ctx *ctx,
	const char *lfn);
/**
 * Destroys a file and replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The FRCEntry to be disposed.
 */
void glite_catalog_FRCEntry_free(glite_catalog_ctx *ctx,
	glite_catalog_FRCEntry *entry);
/**
 * Destroys an array of file and replica catalog entries. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param nitems Number of items in the array.
 * @param entries The array of FRCEntries to be disposed.
 */
void glite_catalog_FRCEntry_freeArray(glite_catalog_ctx *ctx, int nitems,
	glite_catalog_FRCEntry *entries[]);
/**
 * Clones a file and replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param orig The FRCEntry to be cloned.
 *
 * @return The cloned FRCEntry.
 */
glite_catalog_FRCEntry *glite_catalog_FRCEntry_clone(glite_catalog_ctx *ctx,
	glite_catalog_FRCEntry *orig);
/**
 * Sets the GUID for a file and replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The FRCEntry to be modified.
 * @param guid The new value of the GUID field.
 *
 * @return 0 when successful and -1 in case of an error. 
 */
int glite_catalog_FRCEntry_setGuid(glite_catalog_ctx *ctx,
	glite_catalog_FRCEntry *entry, const char *guid);
/**
 * Adds a replica URL to a file and replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The FRCEntry to be modified.
 * @param surl The new value of the SURL field. 
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_FRCEntry_addSurl(glite_catalog_ctx *ctx,
	glite_catalog_FRCEntry *entry, const glite_catalog_SURLEntry *surl);
/**
 * Sets the checksum of a file and replica catalog entry. 
 *
 * @param ctx The general catalog-context, can be NULL if no error reporting is required. 
 * @param entry The FRCEntry to be modified.
 * @param checksum The new checksum value. 
 *
 * @return 0 when successful and -1 in case of an error.
 */
int glite_catalog_FRCEntry_setChecksum(glite_catalog_ctx *ctx,
	glite_catalog_FRCEntry *entry, const char *checksum);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* GLITE_DATA_CATALOG_SIMPLE_H */
