/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Management - Simple encrypted data storage API implementation
 *
 *  Authors:
 *        Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *        Patrick Guio <patrick.guio@bccs.uib.no>
 *
 */

/* asprintf() is GNU extension */
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <glite/data/glite-util.h>
#include <glite/data/hydra/c/eds-simple.h>
#include <glite/data/catalog/metadata/c/metadata-simple.h>
#include <ServiceDiscovery.h>
#include <glite/security/ssss.h>


/* Attribute values in Metadata Catalog */
#define EDS_ATTR_IV      "edsiv"
#define EDS_ATTR_KEY     "edskey"
#define EDS_ATTR_CIPHER  "edscipher"
#define EDS_ATTR_KEYINFO "edskeyinfo"
#define EDS_ATTR_KEYSNEEDED "edskeysneeded"
#define EDS_ATTR_KEYINDEX "edskeyindex"

/* Default cipher */
#define EDS_DEFAULT_CIPHER "bf-cbc"

struct hydra_data {
    char *hex_key;
    char *hex_iv;
    char *cipher;
    char *keyinfo;
    int keys_needed;
    int key_index;
};

EVP_CIPHER_CTX *glite_eds_init(char *id, char **key, char **iv,
                               const EVP_CIPHER **type, char **error);


/**
 * Helper function - check the value of an attribute. If not present,
 * return the default value
 */
static char *get_attr_value(glite_catalog_Attribute **attrs, int attrnum,
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
static int to_hex(unsigned char *in, int insize, unsigned char **out)
{
    int i, len;
    const char hex[] = "0123456789ABCDEF";
    
    if (!out)
        return -1;

    len = insize * 2;

    if (NULL == (*out = (unsigned char *)calloc(1, len+1)))
        return -1;

    for (i = 0; i < insize; i++)
    {
        (*out)[i*2] = hex[in[i] >> 4];
        (*out)[i*2+1] = hex[in[i] & 15];
    }
    (*out)[len] = '\0';

    return len;
}

/**
 * Helper function - convert a hexadecimal string to binary data, return is
 * out size or -1 in case of memory allocation error
 */
static int to_bin(unsigned char *in, unsigned char **out)
{
    int i, len;
    const char hex[] = "0123456789abcdef";
    
    if (!out)
        return -1;

    len = strlen(in)/2;

    *out = (unsigned char *)calloc(1, len+1);
    if (!(*out))
        return -1;

    for (i = 0; i < len; i++)
    {
        char *p;
        p = strchr(hex, (in[i*2] | 0x20)); /* lowercased */
        (*out)[i] = (p-hex)*16;
        p = strchr(hex, (in[i*2+1] | 0x20)); /* lowercased */
        (*out)[i] += (p-hex);
    }
    (*out)[len] = '\0';

    return len;
}

/**
 * Helper function - Convert unsigned integer string to int.
 * Returns < 0 in error cases.
 */
static int ustrtoi(char * str) 
{
    char *end;
    int res;

    if (!str || !(*str))
	return -1;
 
    res = (int)strtol(str, &end, 10);
    if (*end != '\0')
        return -1;

    return res;
}

/**
 * Helper function - Free the char* array.
 */
static void free_str_list(char **array, int count)
{
    int i;

    for(i = 0; i < count; i++)
        free(array[i]);

    free(array);
}

/**
 * Helper function - (Re)allocate and initialize char* array.
 * list is the pointer to the current array (or NULL),
 * *count is current size of the list (IN/OUT).
 */
static unsigned char ** realloc_str_list(unsigned char ** list, unsigned int *count,
    unsigned int new_count)
{
    unsigned int i;

    list = realloc(list, sizeof(unsigned char **) * new_count);
    if (!list)
        return NULL;

    for(i = *count; i < new_count; i++)
        list[i] = NULL; 

    *count = new_count;
    return list;
}

/**
 * Helper function - register data to the single named metadata catalog
 */
static int glite_eds_put_metadata_single(char *endpoint, char *id,
    const struct hydra_data *data, char **error)
{
    glite_catalog_ctx *ctx;
    char keysneeded_str[10], keyindex_str[10];
    const glite_catalog_Attribute iv_attr = {EDS_ATTR_IV, data->hex_iv, NULL};
    const glite_catalog_Attribute key_attr = {EDS_ATTR_KEY, data->hex_key, NULL};
    const glite_catalog_Attribute cipher_attr = {EDS_ATTR_CIPHER, data->cipher, NULL};
    const glite_catalog_Attribute keyinfo_attr = {EDS_ATTR_KEYINFO, data->keyinfo, NULL};
    const glite_catalog_Attribute keysneeded_attr = {EDS_ATTR_KEYSNEEDED, keysneeded_str, NULL};
    const glite_catalog_Attribute keyindex_attr = {EDS_ATTR_KEYINDEX, keyindex_str, NULL};
    const glite_catalog_Attribute *attrs[] = {&cipher_attr, &key_attr,
        &iv_attr, &keyinfo_attr, &keysneeded_attr, &keyindex_attr};
    const int attrs_count = sizeof(attrs)/sizeof(*attrs);

    snprintf(keysneeded_str, sizeof(keysneeded_str), "%d", data->keys_needed);
    snprintf(keyindex_str, sizeof(keyindex_str), "%d", data->key_index);
 
    if (NULL == (ctx = glite_catalog_new(endpoint)))
    {
        asprintf(error, "glite_eds_put_metadata_single error (init): %s", glite_catalog_get_error(NULL));
        return -1;
    }
    if (glite_metadata_createEntry(ctx, id, "eds"))
    {
        asprintf(error, "glite_eds_put_metadata_single error (createEntry): %s", glite_catalog_get_error(ctx));
        glite_catalog_free(ctx);
        return -1;
    }

    if (glite_metadata_setAttributes(ctx, id, attrs_count, attrs))
    {
        asprintf(error, "glite_eds_put_metadata_single error (setAttributes): %s", glite_catalog_get_error(ctx));
        glite_catalog_free(ctx);
        return -1;
    }
    glite_catalog_free(ctx);

    return 0;
}

/**
 * Helper function - get metadata related to the id from the single endpoint.
 */
static int glite_eds_get_metadata_single(char *endpoint, char *id,
    struct hydra_data *data, char **error)
{
    glite_catalog_ctx *ctx;
    glite_catalog_Attribute **result;
    int result_cnt;
    const char *attrs[] = {EDS_ATTR_IV, EDS_ATTR_KEY, EDS_ATTR_CIPHER, EDS_ATTR_KEYINFO,
                           EDS_ATTR_KEYSNEEDED, EDS_ATTR_KEYINDEX};
    const int attrs_count = sizeof(attrs)/sizeof(*attrs);
    char *keysneeded_str, *keyindex_str;

    /* Get Metadata Catalog attributes for the file */
    ctx = glite_catalog_new(endpoint);
    if (!ctx)
    {
        asprintf(error, "glite_eds_init error: %s", glite_catalog_get_error(NULL));
        return -1;
    }

    result = glite_metadata_getAttributes(ctx, id, attrs_count, attrs, &result_cnt);
    if (result_cnt < 0)
    {
        asprintf(error, "glite_eds_init error: %s", glite_catalog_get_error(ctx));
        glite_catalog_free(ctx);
        return -1;
    }

    data->hex_iv = get_attr_value(result, result_cnt, EDS_ATTR_IV, NULL);
    data->hex_key = get_attr_value(result, result_cnt, EDS_ATTR_KEY, NULL);
    data->keyinfo = get_attr_value(result, result_cnt, EDS_ATTR_KEYINFO, NULL);
    data->cipher = get_attr_value(result, result_cnt, EDS_ATTR_CIPHER, NULL);

    keysneeded_str = get_attr_value(result, result_cnt, EDS_ATTR_KEYSNEEDED, NULL);
    data->keys_needed = ustrtoi(keysneeded_str);
    free(keysneeded_str);

    keyindex_str = get_attr_value(result, result_cnt, EDS_ATTR_KEYINDEX, NULL);
    data->key_index = ustrtoi(keyindex_str);
    free(keyindex_str);

    glite_catalog_Attribute_freeArray(ctx, result_cnt, result);
    glite_catalog_free(ctx);

    /* Check required attributes */
    if (!data->hex_iv || !data->hex_key || !data->keyinfo || 
        !data->cipher || data->keys_needed < 0 || data->key_index < 0) {
        asprintf(error, "glite_eds_get_metadata_single: required attributes missing");
        free(data->hex_iv);
        free(data->hex_key);
        free(data->keyinfo);
        free(data->cipher);
        return -1;
    }
    
    return 0;
}

/**
 * Unregister catalog entries in case of error (key/iv)
 * from the single endpoint.
 */
static int glite_eds_unregister_single(char *endpoint, char *id, char **error)
{
    glite_catalog_ctx *ctx;
    if (NULL == (ctx = glite_catalog_new(endpoint)))
    {
        asprintf(error, "glite_eds_unregister_single error: %s", glite_catalog_get_error(NULL));
        return -1;
    }

    if (glite_metadata_removeEntry(ctx, id))
    {
        asprintf(error, "glite_eds_unregister_single error: %s", glite_catalog_get_error(ctx));
        return -1;
    }

    return 0;
}

/**
 * Get endpoints of default catalog service and all associated services.
 * User should free the list (or error string) after use.
 */
char ** glite_eds_get_catalog_endpoints(int *epcount, char **error)
{
    SDService *service;
    SDServiceList * serv_list;
    SDException exc;
    const char *sd_type;
    char * serv_name;
    char **endpoints;
    int associated_count;
    int i;

    sd_type = getenv(GLITE_METADATA_SD_ENV);
    if (!sd_type) sd_type = GLITE_METADATA_SD_TYPE;

    serv_name = glite_discover_service_by_version(sd_type, NULL /*name*/, NULL /*version*/, error);
    if (!serv_name)
        return NULL;

    service = SD_getService(serv_name, &exc);
    if (!service) {
        asprintf(error, "glite_eds_get_catalog_endpoints: %s", exc.reason);
	SD_freeException(&exc);
	free(serv_name);
        return NULL;
    }

    /* Get list of (associated) services.
     * NULL is returned also in case of numServices=0, that is not the error here.
     */
    serv_list = SD_listAssociatedServices(serv_name, sd_type, NULL/*site*/, NULL/*vos*/, &exc);

    if (!serv_list)
	associated_count = 0;
    else
        associated_count = serv_list->numServices;

    /* Create list of endpoints */

    endpoints = malloc(sizeof(char *) * (1 + associated_count));
    if (!endpoints) {
        SD_freeService(service);
        SD_freeServiceList(serv_list);
	free(serv_name);
        asprintf(error, "glite_eds_get_catalog_endpoints: out of memory");
	return NULL;
    }

    endpoints[0] = strdup(service->endpoint);
    for (i = 0; i < associated_count; i++) {
        endpoints[i+1] = strdup(serv_list->services[i]->endpoint);
    }

    /* free resources */
    SD_freeService(service);
    SD_freeServiceList(serv_list);
    free(serv_name);

    *epcount = 1 + associated_count;
    return endpoints;
}

/**
 * Helper function - register datas to metadata catalog(s).
 * The key is splitted before the storage.
 */
static int glite_eds_put_metadata(char *id, char *hex_key, char *hex_iv, char *cipher,
    char *keyinfo, char **error)
{
    char **endpoints;
    int epcount;
    unsigned char ** key_list;
    unsigned int keys_needed;
    int i;
    int err = 0;

    endpoints = glite_eds_get_catalog_endpoints(&epcount, error);
    if (!endpoints)
        return -1;

    /* Calculate minimum count of pieces required to reconstruct the original key */
    for(keys_needed = 0, i = epcount; i ; i/=2)
	keys_needed += 1;

    /* Split the key by Shamir's Secret Sharing Scheme. */
    key_list = glite_security_ssss_split_key(hex_key, epcount, keys_needed);
    if (!key_list) {
        asprintf(error, "glite_eds_put_metadata error: ssss_split failed");
        free_str_list(endpoints, epcount);
        return -1;
    }

    /* Save each key piece to different catalog. */
    for (i = 0; i < epcount; i++) {
        const struct hydra_data data = {
            .hex_key = key_list[i],
            .hex_iv = hex_iv,
            .cipher = cipher,
            .keyinfo = keyinfo,
            .keys_needed = keys_needed,
            .key_index = i };
        err = glite_eds_put_metadata_single(endpoints[i], id, &data, error);
        if (err) 
            break;
    }

    /* If the storage of any of the key pieces failed, then we
     * should attempt to remove already committed pieces.  */
    if (err) {
        for(i=i-1; i >= 0; i--) {
           char *dummy_error = NULL;
           if (glite_eds_unregister_single(endpoints[i], id, &dummy_error))
             free(dummy_error);
        }
    }

    /* cleanup */
    free_str_list(endpoints, epcount);
    free_str_list((char**)key_list, epcount);

    return err;
}

/**
 * Helper function - get and join metadata related to the id.
 */
static int glite_eds_get_metadata(char *id, char **hex_key, char **hex_iv, char **cipher,
    char **keyinfo, char **error)
{
    char **endpoints;
    int epcount;
    unsigned char ** key_list;
    struct hydra_data data;
    unsigned int keys_needed = 0;
    unsigned int key_shares = 0;
    unsigned int keycount = 0;
    char *err;
    int i;

    endpoints = glite_eds_get_catalog_endpoints(&epcount, error);
    if (!endpoints)
        return -1;

    /* default key_shares to numServices. Note: it's possible that it's not
     * the same as original key_shares if the number of the services has
     * been changed later! */
    key_list = realloc_str_list(NULL, &key_shares, epcount);
    if (!key_list) {
        asprintf(error, "glite_eds_get_metadata: out of memory");
        free_str_list(endpoints, epcount);
        return -1;
    }

    /* Fetch each key piece from separate catalog. */
    *error = NULL;
    for (i = 0; i < epcount; i++) {
        if (glite_eds_get_metadata_single(endpoints[i], id, &data, &err)) {
	    /* Keep first error only */
            if (*error == NULL) *error = err;
            else free(err);
        } else {
            /* Realloc list if key_index is greater than expected. */
            if ((unsigned int)data.key_index >= key_shares)
            {
                key_list = realloc_str_list(key_list, &key_shares, data.key_index);
                if (!key_list) {
                    free_str_list(endpoints, epcount);
                    asprintf(error, "glite_eds_get_metadata: out of memory");
                    return -1;
                }
            }

            /* Save one key piece. */
            key_list[data.key_index] = strdup(data.hex_key);

	    /* Save common data from first entry and
             * make some cross checks for the rest. */
	    if (keycount == 0) {
                keys_needed = data.keys_needed;
                *hex_iv = strdup(data.hex_iv);
                *keyinfo = strdup(data.keyinfo);
                *cipher = strdup(data.cipher);
            } else if ((unsigned int)data.keys_needed != keys_needed ||
                strcmp(data.hex_iv, *hex_iv) ||
                strcmp(data.keyinfo, *keyinfo) ||
                strcmp(data.cipher, *cipher))
            {
                free(*error);
                asprintf(error, "glite_eds_get_metadata: metadata corrupted");
                keys_needed = 0; /* break the loop */
            }
            keycount++;
           
            free(data.hex_iv);
            free(data.hex_key);
            free(data.keyinfo);
            free(data.cipher);

            /* Don't continue if we have enough pieces */
            if (keycount >= keys_needed)
                break;
        }
    }

    free_str_list(endpoints, epcount);

    if (!keys_needed || keycount < keys_needed) {
        if (*error == NULL) 
            asprintf(error, "glite_eds_get_metadata: failed to get all key pieces");
        free_str_list((char**)key_list, key_shares);
        return -1;
    }
    
    free(*error);

    /* Join ssss key pieces */ 
    *hex_key = glite_security_ssss_join_keys(key_list, key_shares);
    free_str_list((char**)key_list, key_shares);
    
    if (! (*hex_key)) {
        asprintf(error, "glite_eds_get_metadata: Error join keys");
        return -1;
    }
  
    return 0;
}

/**
 * Helper function - used by glite_eds_encrypt_init and glite_eds_decrypt_init
 */
EVP_CIPHER_CTX *glite_eds_init(char *id, char **key, char **iv,
    const EVP_CIPHER **type, char **error)
{
    EVP_CIPHER_CTX *ectx;
    char *cipher_name, *keyinfo, *hex_key, *hex_iv;

    if (glite_eds_get_metadata(id, &hex_key, &hex_iv, &cipher_name, &keyinfo, error))
        return NULL;

    to_bin(hex_key, (unsigned char **)key);
    to_bin(hex_iv, (unsigned char **)iv);
 
    if (!RAND_load_file("/dev/random", 1))
    {
        asprintf(error, "glite_eds_init error: %s",
            ERR_error_string(ERR_get_error(), NULL));
        return NULL;
    }

    OpenSSL_add_all_ciphers();

    *type = EVP_get_cipherbyname(cipher_name);
    if (!(*type))
    {
        asprintf(error, "glite_eds_init error: %s",
            ERR_error_string(ERR_get_error(), NULL));
        return NULL;
    }

    ectx = (EVP_CIPHER_CTX *)malloc(sizeof(EVP_CIPHER_CTX));
    if (!ectx)
    {
        asprintf(error, "glite_eds_init error: calloc() of %d "
            "bytes failed", sizeof(*ectx));
        return NULL;
    }

    EVP_CIPHER_CTX_init(ectx);

    free(cipher_name); free(keyinfo); free(hex_key); free(hex_iv);

    return ectx;
}

static int _glite_eds_register_common(char *id, char * cipher, int keysize,
    char **key_p, char **iv_p, const EVP_CIPHER **type_p, char **error)
{
    char *cipher_to_use, *keyl_str;
    unsigned char *hex_key, *hex_iv;
    int keyLength, ivLength;
    int res;

    /* Do OpenSSL cipher initialization */
    if (!RAND_load_file("/dev/random", 1))
    {
        asprintf(error, "glite_eds_register error: %s",
            ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }
    OpenSSL_add_all_ciphers();
    cipher_to_use = (cipher) ? cipher : EDS_DEFAULT_CIPHER;
    if (0 == ((*type_p) = EVP_get_cipherbyname(cipher_to_use)))
    {
        asprintf(error, "glite_eds_register error: %s",
            ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    /* Initialize encryption key and initialization vector */
    ivLength = EVP_CIPHER_iv_length(*type_p);
    keyLength = (keysize) ? (keysize >> 3) : EVP_CIPHER_key_length(*type_p);
    if (NULL == (*iv_p = (char *)malloc(ivLength)))
    {
        asprintf(error, "glite_eds_register error: malloc() of %d bytes "
            "failed", ivLength);
        return -1;
    }
    if (NULL == (*key_p = (char *)malloc(keyLength)))
    {
        asprintf(error, "glite_eds_register error: malloc() of %d bytes "
            "failed", keyLength);
        return -1;
    }
    RAND_bytes((char *)*key_p, keyLength);
    if (keyLength * 2 != to_hex(*key_p, keyLength, &hex_key))
    {
        asprintf(error, "glite_eds_register error: converting key to hex "
            "format failed");
        return -1;
    }
    RAND_pseudo_bytes((char *)*iv_p, ivLength);
    if (ivLength * 2 != to_hex(*iv_p, ivLength, &hex_iv))
    {
        asprintf(error, "glite_eds_register error: converting iv to hex "
            "format failed");
        return -1;
    }

    /* Do the Metadata Catalog stuff */
    asprintf(&keyl_str, "%d", keyLength<<3);
    res = glite_eds_put_metadata(id, hex_key, hex_iv, cipher_to_use, keyl_str, error);

    free(hex_iv); free(hex_key); free(keyl_str);
    
    return res;
}

/**
 * Register a new file in Hydra: create metadata entries (key/iv/...)
 */
int glite_eds_register(char *id, char *cipher, int keysize, char **error)
{
    char *key, *iv;
    const EVP_CIPHER *type;
    int ret = -1;

    ret = _glite_eds_register_common(id, cipher, keysize,
        &key, &iv, &type, error);

    free(key); free(iv);
    return ret;
}

/**
 * Register a new file in Hydra: create metadata entries (key/iv/...),
 * initalizes encryption context
 */
EVP_CIPHER_CTX *glite_eds_register_encrypt_init(char *id,
    char *cipher, int keysize, char **error)
{
    char *key, *iv;
    EVP_CIPHER_CTX *ectx;
    const EVP_CIPHER *type;
    int ret = -1;

    ret = _glite_eds_register_common(id, cipher, keysize,
        &key, &iv, &type, error);

    if(ret == -1) return NULL;

    if (NULL == (ectx = (EVP_CIPHER_CTX *)calloc(1, sizeof(*ectx))))
    {
        asprintf(error, "glite_eds_register_encrypt_init error: calloc() of %d "
            "bytes failed", sizeof(*ectx));
        return NULL;
    }
    EVP_CIPHER_CTX_init(ectx);
    EVP_EncryptInit(ectx, type, key, iv);

    free(key); free(iv);

    return ectx;
}

/**
 * Initialize encryption context for a file. Query key/iv pairs from
 * metadata catalog
 */
EVP_CIPHER_CTX *glite_eds_encrypt_init(char *id, char **error)
{
    char *iv, *key;
    const EVP_CIPHER *type;
    EVP_CIPHER_CTX *ectx;
    
    ectx = glite_eds_init(id, &key, &iv, &type, error);
    if (!ectx)
        return NULL;

    EVP_EncryptInit(ectx, type, key, iv);
    
    free(key); free(iv);

    return ectx;
}

/**
 * Initialize decryption context for a file. Query key/iv pairs from
 * metadata catalog
 */
EVP_CIPHER_CTX *glite_eds_decrypt_init(char *id, char **error)
{
    char *iv, *key;
    const EVP_CIPHER *type;
    EVP_CIPHER_CTX *dctx;
    
    dctx = glite_eds_init(id, &key, &iv, &type, error);
    if (!dctx)
        return NULL;

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
    int enc_buffer_size;
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
    int enc_buffer_size;
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
    int dec_buffer_size;
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
    *error = NULL;
    return 0;
}

/**
 * Unregister catalog entries in case of error (key/iv)
 */
int glite_eds_unregister(char *id, char **error)
{
    char **endpoints;
    int epcount;
    int i;
    int res = 0;

    endpoints = glite_eds_get_catalog_endpoints(&epcount, error);
    if (!endpoints)
        return -1;

    /* Remove the entry from each catalog. */
    for (i = 0; i < epcount; i++) {
        res |= glite_eds_unregister_single(endpoints[i], id, error);
    }

    free_str_list(endpoints, epcount);

    return res;
}

