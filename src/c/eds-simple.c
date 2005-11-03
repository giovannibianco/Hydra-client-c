/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Management - Simple encrypted data storage API implementation
 *
 *  Authors:
 *	Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *	Patrick Guio <patrick.guio@bccs.uib.no>
 *
 */

#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <glite/data/hydra/c/eds-simple.h>
#include <glite/data/catalog/fireman/c/fireman-simple.h>
#include <glite/data/catalog/metadata/c/metadata-simple.h>


/* Attribute values in Metadata Catalog */
#define EDS_ATTR_IV      "edsiv"
#define EDS_ATTR_KEY     "edskey"
#define EDS_ATTR_CIPHER  "edscipher"
#define EDS_ATTR_KEYINFO "edskeyinfo"

/* Default cipher */
#define EDS_DEFAULT_CIPHER "bf-cbc"


/**
 * Helper function - check the value of an attribute. If not present,
 * return the default value
 */
char *get_attr_value(glite_catalog_Attribute **attrs, int attrnum,
    const char *name, const char *def_val)
{
    int i;
    char *retval = NULL;

    for (i = 0; i < attrnum; i++)
    {
	if (!strcmp(attrs[i]->name, name))
	    if (attrs[i]->value)
	    {
		retval = strdup(attrs[i]->value);
		break;
	    }
    }
    if (!retval && def_val)
	retval = strdup(def_val);

    return retval;
}

/**
 * Helper function - convert binary data to hexadecimal format, return is out
 * size or -1 in case of memory allocation error
 */
int to_hex(unsigned char *in, int insize, unsigned char **out)
{
    int i, ret = -1;
    const char hex[] = "0123456789ABCDEF";
    
    if (!out)
	return ret;

    if (NULL == (*out = (unsigned char *)calloc(1, insize*2+1)))
	return ret;
    ret = insize * 2;

    for (i = 0; i < insize; i++)
    {
        (*out)[i*2] = hex[in[i] >> 4];
        (*out)[i*2+1] = hex[in[i] & 15];
    }

    return ret;
}

/**
 * Helper function - convert a hexadecimal string to binary data, return is
 * out size or -1 in case of memory allocation error
 */
int to_bin(unsigned char *in, unsigned char **out)
{
    int i, ret = -1;
    const char hex[] = "0123456789ABCDEF";
    
    if (!out)
        return ret;

    if (NULL == (*out = (unsigned char *)calloc(1, strlen(in)/2)))
        return ret;
    ret = strlen(in)/2;

    for (i = 0; i < ret; i++)
    {
        char *p;
	p = strchr(hex, in[i*2]);
	(*out)[i] = (p-hex)*16;
	p = strchr(hex, in[i*2+1]);
	(*out)[i] += (p-hex);
    }

    return ret;
}

/**
 * Helper function - register datas in the metadata catalog
 */
int glite_eds_put_metadata(char *lfn, char *hex_key, char *hex_iv, char *cipher,
    char *keyinfo, char **error)
{
    glite_catalog_ctx *ctx;
    const glite_catalog_Attribute iv_attr = {EDS_ATTR_IV, hex_iv, NULL};
    const glite_catalog_Attribute key_attr = {EDS_ATTR_KEY, hex_key, NULL};
    const glite_catalog_Attribute cipher_attr = {EDS_ATTR_CIPHER, cipher, NULL};
    const glite_catalog_Attribute keyinfo_attr = {EDS_ATTR_KEYINFO, keyinfo, NULL};
    const glite_catalog_Attribute *attrs[] = {&cipher_attr, &key_attr,
	&iv_attr, &keyinfo_attr};

    if (NULL == (ctx = glite_catalog_new(NULL)))
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_put_metadata error: %s", catalog_err);
	return -1;
    }
    if (glite_metadata_createEntry(ctx, lfn, "eds"))
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_put_metadata error: %s", catalog_err);
	return -1;
    }
    if (glite_metadata_setAttributes(ctx, lfn, 4, attrs))
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_put_metadata error: %s", catalog_err);
	return -1;
    }
    glite_catalog_free(ctx);

    return 0;
}

/**
 * Helper function - register a new file in fireman catalog
 */
int glite_eds_put_fireman(char *lfn, char *id, char **error)
{
    const char *SURL_prefix = "srm://";
    glite_catalog_ctx *ctx;
 
    if (NULL == (ctx = glite_catalog_new(NULL)))
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_put_fireman error: %s", catalog_err);
	return -1;
    }

    if (!strncmp(id, SURL_prefix, strlen(SURL_prefix)))
    {
	glite_catalog_SURLEntry *surl_entry;
	glite_catalog_FRCEntry *rentry;
	
	surl_entry = glite_catalog_SURLEntry_new(NULL, id, 1);
	rentry = glite_catalog_FRCEntry_new(ctx, lfn);
	glite_catalog_FRCEntry_addSurl(ctx, rentry, surl_entry);
	
	if (glite_fireman_create(ctx, rentry))
	{
	    const char *catalog_err;
	    catalog_err = glite_catalog_get_error(ctx);
	    
	    asprintf(error, "glite_eds_put_fireman error: %s", catalog_err);
	    return -1;
	}
    }
    else
    {
 	glite_catalog_FRCEntry *entry;
	
	entry = glite_catalog_FRCEntry_new(ctx, lfn);
	glite_catalog_FRCEntry_setGuid(ctx, entry, id);

 	if (glite_fireman_create(ctx, entry))
	{
	    const char *catalog_err;
	    catalog_err = glite_catalog_get_error(ctx);
	    
	    asprintf(error, "glite_eds_put_fireman error: %s", catalog_err);
	    return -1;
	}
    }

    glite_catalog_free(ctx);
    
    return 0;
}

/**
 * Helper funcition - used by glite_eds_encrypt_init and glite_eds_decrypt_init
 */
EVP_CIPHER_CTX *glite_eds_init(char *lfn, char **key, char **iv,
    const EVP_CIPHER **type, char **error)
{
    int result_cnt;
    EVP_CIPHER_CTX *ectx;
    glite_catalog_ctx *ctx;
    glite_catalog_Attribute **result;
    char *cipher_name, *keyinfo, *hex_key, *hex_iv;
    const char *attrs[] = {EDS_ATTR_IV, EDS_ATTR_KEY, EDS_ATTR_CIPHER, EDS_ATTR_KEYINFO};
    
    /* Get Metadata Catalog attributes for the file */
    if (NULL == (ctx = glite_catalog_new(NULL)))
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_init error: %s", catalog_err);
	return NULL;
    }

    result = glite_metadata_getAttributes(ctx, lfn, 4, attrs, &result_cnt);
    if (result_cnt < 0)
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_init error: %s", catalog_err);
	return NULL;
    }

    /* Check the presence of all required attributes */
    cipher_name = get_attr_value(result, result_cnt, EDS_ATTR_CIPHER, NULL);
    if (!cipher_name)
    {
	asprintf(error, "glite_eds_init error: attribute %s not found "
	    "in metadata catalog", EDS_ATTR_CIPHER);
	return NULL;
    }
    hex_iv = get_attr_value(result, result_cnt, EDS_ATTR_IV, NULL);
    if (!hex_iv)
    {
	asprintf(error, "glite_eds_init error: attribute %s not found "
	    "in metadata catalog", EDS_ATTR_IV);
	return NULL;
    }
    hex_key = get_attr_value(result, result_cnt, EDS_ATTR_KEY, NULL);
    if (!hex_key)
    {
	asprintf(error, "glite_eds_init error: attribute %s not found "
	    "in metadata catalog", EDS_ATTR_KEY);
	return NULL;
    }
    keyinfo = get_attr_value(result, result_cnt, EDS_ATTR_KEYINFO, NULL);
    if (!keyinfo)
    {
	asprintf(error, "glite_eds_init error: attribute %s not found "
	    "in metadata catalog", EDS_ATTR_KEYINFO);
	return NULL;
    }
    glite_catalog_Attribute_freeArray(ctx, result_cnt, result);
    glite_catalog_free(ctx);

    to_bin(hex_key, (unsigned char **)key);
    to_bin(hex_iv, (unsigned char **)iv);

    if (!RAND_load_file("/dev/random", 1))
    {
	asprintf(error, "glite_eds_init error: %s",
	    ERR_error_string(ERR_get_error(), NULL));
	return NULL;
    }
    OpenSSL_add_all_ciphers();
    if (0 == (*type = EVP_get_cipherbyname(cipher_name)))
    {
	asprintf(error, "glite_eds_init error: %s",
	    ERR_error_string(ERR_get_error(), NULL));
	return NULL;
    }

    if (NULL == (ectx = (EVP_CIPHER_CTX *)calloc(1, sizeof(*ectx))))
    {
	asprintf(error, "glite_eds_init error: calloc() of %d "
	    "bytes failed", sizeof(*ectx));
	return NULL;
    }
    EVP_CIPHER_CTX_init(ectx);

    free(cipher_name); free(keyinfo); free(hex_key); free(hex_iv);

    return ectx;
}

/**
 * Register a new file in Hydra: create metadata entries (key/iv/...)
 */
int glite_eds_register(char *lfn, char *id, char *cipher, int keysize,
    char **error)
{
    char *key, *iv, *cipher_to_use, *hex_key, *hex_iv, *keyl_str;
    int keyLength, ivLength;
    const EVP_CIPHER *type;

    /* Do OpenSSL cipher initialization */
    if (!RAND_load_file("/dev/random", 1))
    {
	asprintf(error, "glite_eds_register error: %s",
	    ERR_error_string(ERR_get_error(), NULL));
	return -1;
    }
    OpenSSL_add_all_ciphers();
    cipher_to_use = (cipher) ? cipher : EDS_DEFAULT_CIPHER;
    if (0 == (type = EVP_get_cipherbyname(cipher_to_use)))
    {
	asprintf(error, "glite_eds_register error: %s",
	    ERR_error_string(ERR_get_error(), NULL));
	return -1;
    }

    /* Initialize encryption key and initialization vector */
    ivLength = EVP_CIPHER_iv_length(type);
    keyLength = (keysize) ? (keysize >> 3) : EVP_CIPHER_key_length(type);
    if (NULL == (iv = (char *)malloc(ivLength)))
    {
	asprintf(error, "glite_eds_register error: malloc() of %d bytes "
	    "failed", ivLength);
	return -1;
    }
    if (NULL == (key = (char *)malloc(keyLength)))
    {
	asprintf(error, "glite_eds_register error: malloc() of %d bytes "
	    "failed", keyLength);
	return -1;
    }
    RAND_bytes((char *)key, keyLength);
    if (keyLength * 2 != to_hex(key, keyLength, (unsigned char **)&hex_key))
    {
        asprintf(error, "glite_eds_register error: converting key to hex "
	    "format failed");
	return -1;
    }
    RAND_pseudo_bytes((char *)iv, ivLength);
    if (ivLength * 2 != to_hex(iv, ivLength, (unsigned char **)&hex_iv))
    {
        asprintf(error, "glite_eds_register error: converting iv to hex "
	    "format failed");
	return -1;
    }

    /* Do the Metadata Catalog stuff */
    asprintf(&keyl_str, "%d", keyLength<<3);
    if (glite_eds_put_metadata(lfn, hex_key, hex_iv, cipher_to_use, keyl_str, error))
    {
	return -1;
    }

    /* If id (SURL/GUID) is present, create Fireman Catalog entry */
    if (id)
    {
        if (glite_eds_put_fireman(lfn, id, error))
	{
	    return -1;
	}
    }


    free(iv); free(hex_iv); free(key); free(hex_key); free(keyl_str);
    
    return 0;
}

/**
 * Register a new file in Hydra: create metadata entries (key/iv/...),
 * initalizes encryption context
 */
EVP_CIPHER_CTX *glite_eds_register_encrypt_init(char *lfn, char *id,
    char *cipher, int keysize, char **error)
{
    char *key, *hex_key, *iv, *hex_iv, *cipher_to_use, *keyl_str;
    int keyLength, ivLength;
    EVP_CIPHER_CTX *ectx;
    const EVP_CIPHER *type;

    /* Do OpenSSL cipher initialization */
    if (!RAND_load_file("/dev/random", 1))
    {
	asprintf(error, "glite_eds_register_encrypt_init error: %s",
	    ERR_error_string(ERR_get_error(), NULL));
	return NULL;
    }
    OpenSSL_add_all_ciphers();
    cipher_to_use = (cipher) ? cipher : EDS_DEFAULT_CIPHER;
    if (0 == (type = EVP_get_cipherbyname(cipher_to_use)))
    {
	asprintf(error, "glite_eds_register_encrypt_init error: %s",
	    ERR_error_string(ERR_get_error(), NULL));
	return NULL;
    }

    if (NULL == (ectx = (EVP_CIPHER_CTX *)calloc(1, sizeof(*ectx))))
    {
	asprintf(error, "glite_eds_register_encrypt_init error: calloc() of %d "
	    "bytes failed", sizeof(*ectx));
	return NULL;
    }
    EVP_CIPHER_CTX_init(ectx);
    EVP_EncryptInit(ectx, type, NULL, NULL);

    /* Initialize encryption key and initialization vector */
    ivLength = EVP_CIPHER_iv_length(type);
    keyLength = (keysize) ? (keysize >> 3) : EVP_CIPHER_key_length(type);
    if (NULL == (iv = (char *)malloc(ivLength)))
    {
	asprintf(error, "glite_eds_register_encrypt_init error: malloc() of %d "
	    "bytes failed", ivLength);
	free(ectx);
	return NULL;
    }
    if (NULL == (key = (char *)malloc(keyLength)))
    {
	asprintf(error, "glite_eds_register_encrypt_init error: malloc() of %d "
	    "bytes failed", keyLength);
	free(ectx);
	return NULL;
    }
    RAND_bytes((char *)key, keyLength);

    if (keyLength * 2 != to_hex(key, keyLength, (unsigned char **)&hex_key))
    {
        asprintf(error, "glite_eds_register error: converting key to hex "
	    "format failed");
	free(ectx);
	return NULL;
    }
    RAND_pseudo_bytes((char *)iv, ivLength);
    if (ivLength * 2 != to_hex(iv, ivLength, (unsigned char **)&hex_iv))
    {
        asprintf(error, "glite_eds_register error: converting iv to hex "
	    "format failed");
	free(ectx);
	return NULL;
    }

    EVP_EncryptInit(ectx, NULL, key, iv);

    /* Do the Metadata Catalog stuff */
    asprintf(&keyl_str, "%d", keyLength << 3);
    if (glite_eds_put_metadata(lfn, hex_key, hex_iv, cipher_to_use, keyl_str, error))
    {
        free(ectx);
	return NULL;
    }
    free(keyl_str);

    /* If id (SURL/GUID) is present, create Fireman Catalog entry */
    if (id)
    {
        if (glite_eds_put_fireman(lfn, id, error))
	{
	    free(ectx);
	    return NULL;
	}
    }

    return ectx;
}

/**
 * Initialize encryption context for a file. Query key/iv pairs from
 * metadata catalog
 */
EVP_CIPHER_CTX *glite_eds_encrypt_init(char *lfn, char **error)
{
    char *iv, *key;
    const EVP_CIPHER *type;
    EVP_CIPHER_CTX *ectx;
    
    if (NULL == (ectx = glite_eds_init(lfn, &key, &iv, &type, error)))
    {
	return NULL;
    }

    EVP_EncryptInit(ectx, type, key, iv);
    free(key); free(iv);

    return ectx;
}

/**
 * Initialize decryption context for a file. Query key/iv pairs from
 * metadata catalog
 */
EVP_CIPHER_CTX *glite_eds_decrypt_init(char *lfn, char **error)
{
    char *iv, *key;
    const EVP_CIPHER *type;
    EVP_CIPHER_CTX *dctx;
    
    if (NULL == (dctx = glite_eds_init(lfn, &key, &iv, &type, error)))
    {
	return NULL;
    }

    EVP_DecryptInit(dctx, type, key, iv);
    free(key); free(iv);

    return dctx;
}

/**
 * Encrypts a memory block using the encryption context
 */
int glite_eds_encrypt_block(EVP_CIPHER_CTX *ectx, char *mem_in, int mem_in_size,
    char **mem_out, int *mem_out_size, char **error)
{
    int enc_buffer_size, trial;
    char *enc_buffer;

    enc_buffer = (char *)malloc(mem_in_size + EVP_CIPHER_CTX_block_size(ectx));
    if (!enc_buffer)
    {
	asprintf(error, "glite_eds_encrypt_block error: failed to allocate "
	    "%d bytes of memory", mem_in_size + EVP_CIPHER_CTX_block_size(ectx));
	return -1;
    }

    EVP_EncryptUpdate(ectx, enc_buffer, &enc_buffer_size,
	mem_in, mem_in_size);

    *mem_out = enc_buffer;
    *mem_out_size = enc_buffer_size;

    return 0;
}

/**
 * Finalizes a block encryption
 */
int glite_eds_encrypt_final(EVP_CIPHER_CTX *ectx, char **mem_out, int *mem_out_size, char **error)
{
    int enc_buffer_size, trial;
    char *enc_buffer;

    enc_buffer = (char *)malloc(EVP_CIPHER_CTX_block_size(ectx));
    if (!enc_buffer)
    {
	asprintf(error, "glite_eds_encrypt_final error: failed to allocate "
	    "%d bytes of memory", EVP_CIPHER_CTX_block_size(ectx));
	return -1;
    }

    EVP_EncryptFinal(ectx, enc_buffer, &enc_buffer_size);

    *mem_out = enc_buffer;
    *mem_out_size = enc_buffer_size;

    return 0;
}

/**
 * Decrypts a memory block using the decryption context
 */
int glite_eds_decrypt_block(EVP_CIPHER_CTX *dctx, char *mem_in,  int mem_in_size,
    char **mem_out, int *mem_out_size, char **error)
{
    int dec_buffer_size, trial;
    char *dec_buffer;

    dec_buffer = (char *)malloc(mem_in_size + EVP_CIPHER_CTX_block_size(dctx));
    if (!dec_buffer)
    {
	asprintf(error, "glite_eds_decrypt_block error: failed to allocate "
	    "%d bytes of memory", mem_in_size + EVP_CIPHER_CTX_block_size(dctx));
	return -1;
    }

    EVP_DecryptUpdate(dctx, dec_buffer, &dec_buffer_size,
	mem_in, mem_in_size);

    *mem_out = dec_buffer;
    *mem_out_size = dec_buffer_size;

    return 0;
}

/**
 * Finalizes memory block decryption
 */
int glite_eds_decrypt_final(EVP_CIPHER_CTX *dctx, char **mem_out, int *mem_out_size, char **error)
{
    int dec_buffer_size;
    char *dec_buffer;

    dec_buffer = (char *)malloc(EVP_CIPHER_CTX_block_size(dctx));
    if (!dec_buffer)
    {
	asprintf(error, "glite_eds_decrypt_final error: failed to allocate "
	    "%d bytes of memory", EVP_CIPHER_CTX_block_size(dctx));
	return -1;
    }

    EVP_DecryptFinal(dctx, dec_buffer, &dec_buffer_size);

    *mem_out = dec_buffer;
    *mem_out_size = dec_buffer_size;

    return 0;
}

/**
 * Finalize an encryption/decryption context
 */
int glite_eds_finalize(EVP_CIPHER_CTX *ctx, char **error)
{
    EVP_CIPHER_CTX_cleanup(ctx);
    return 0;
}

/**
 * Unregister catalog entries in case of error (key/iv)
 */
int glite_eds_unregister(char *lfn, char **error)
{
    glite_catalog_ctx *ctx;
    if (NULL == (ctx = glite_catalog_new(NULL)))
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_unregister error: %s", catalog_err);
	return -1;
    }

    if (glite_metadata_removeEntry(ctx, lfn))
    {
	const char *catalog_err;
	catalog_err = glite_catalog_get_error(ctx);

	asprintf(error, "glite_eds_unregister error: %s", catalog_err);
	return -1;
    }

    return 0;
}
