/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005-2007.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or 
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  GLite Data Management - Simple encrypted data storage API
 *
 *  Authors:
 *      Andrei Kruger <andrei.krueger@cern.ch>
 *	    Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *  	Peter Kunszt <Peter.Kunszt@cern.ch>
 *
 */

#ifndef GLITE_DATA_EDS_SIMPLE_H
#define GLITE_DATA_EDS_SIMPLE_H

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get endpoints of default catalog service and all associated services.
 * 
 * @param count [OUT] count of returned endpoints.
 * @param error [OUT] Pointer to the error string. 
 *
 * @return list of endpoints.
 * The caller is responsible for freeing the allocated structure.
 * In error cases the NULL is returned and *error contains the error string.
 * The caller is responsible for freeing the allocated error string.
 */
char ** glite_eds_get_catalog_endpoints(int *count, char **error);

/**
 * Register a new file in Hydra: create key entries (key/iv/...)
 * 
 * @param id The SURL or GUID of the remote file.
 * @param cipher The cipher name to use.
 * @param keysize Key size to use in bits.
 * @param error [OUT] Pointer to the error string.
 *
 * @return 0 in case of no error. In other cases *error contains the error
 *  string. The caller is responsible for freeing the allocated error string.
 */
int glite_eds_register(char *id, char *cipher, int keysize,
    char **error);

/**
 * Register a new file in Hydra: create key entries (key/iv/...),
 * initalizes encryption context
 * 
 * @param id The ID by which the crypt will be registered (remote file name or GUID).
 * @param cipher The cipher name to use.
 * @param keysize Key size to use in bits.
 * @param error [OUT] Pointer to the error string.
 *
 * @return Encryption context in case of no error. In other cases NULL is
 *  returned, and *error contains the error string. The caller is responsible
 *  for freeing the allocated error string.
 */
EVP_CIPHER_CTX *glite_eds_register_encrypt_init(char *id,
    char *cipher, int keysize, char **error);

/**
 * Initialize encryption context for a file. Query key/iv/... from
 * key storage
 *
 * @param id The ID by which the crypt key is stored (remote file name or GUID)
 * @param error [OUT] Pointer to the error string.
 *
 * @return Encryption context in case of no error. In other cases NULL is
 *  returned, and *error contains the error string. The caller is responsible
 *  for freeing the allocated error string.
 */
EVP_CIPHER_CTX *glite_eds_encrypt_init(char *id, char **error); 

/**
 * Initialize decryption context for a file. Query key/iv/... from
 * key storage
 *
 * @param id The ID by which the crypt key is stored (remote file name or GUID).
 * @param error [OUT] Pointer to the error string.
 *
 * @return Decryption context in case of no error. In other cases NULL is
 *  returned, and *error contains the error string. The caller is responsible
 *  for freeing the allocated error string.
 */
EVP_CIPHER_CTX *glite_eds_decrypt_init(char *id, char **error); 

/**
 * Encrypts a memory block using the encryption context
 * 
 * @param ectx Encryption context 
 * @param mem_in Memory block to encrypt
 * @param mem_in_size Memory block size
 * @param mem_out [OUT] Encrypted memory block's address
 * @param mem_out_size [OUT] Encrypted memory block's size
 * @param error [OUT] Pointer to the error string.
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_encrypt_block(EVP_CIPHER_CTX *ectx, char *mem_in, int mem_in_size,
    char **mem_out, int *mem_out_size, char **error);

/**
 * Finalizes memory block encryption
 * 
 * @param ectx Encryption context 
 * @param mem_out [OUT] Encrypted memory block's address
 * @param mem_out_size [OUT] Encrypted memory block's size
 * @param error [OUT] Pointer to the error string.
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_encrypt_final(EVP_CIPHER_CTX *ectx, char **mem_out, int *mem_out_size, char **error);

/**
 * Decrypts a memory block using the encryption context
 * 
 * @param dctx Decryption context 
 * @param mem_in Memory block to decrypt
 * @param mem_in_size Memory block size
 * @param mem_out [OUT] Decrypted memory block's address
 * @param mem_out_size [OUT] Decrypted memory block's size
 * @param error [OUT] Pointer to the error string.
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_decrypt_block(EVP_CIPHER_CTX *dctx, char *mem_in,  int mem_in_size,
    char **mem_out, int *mem_out_size, char **error);

/**
 * Finalizes memory block encryption
 * 
 * @param dctx Decryption context 
 * @param mem_out [OUT] Decrypted memory block's address
 * @param mem_out_size [OUT] Decrypted memory block's size
 * @param error [OUT] Pointer to the error string.
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_decrypt_final(EVP_CIPHER_CTX *dctx, char **mem_out, int *mem_out_size, char **error);

/**
 * Finalize an encryption/decryption context
 *
 * @param ctx Encryption/decryption context
 * @param error [OUT] Pointer to the error string.
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_finalize(EVP_CIPHER_CTX *ctx, char **error);

/**
 * Unregister catalog entries in case of error (key/iv)
 *
 * @param id The ID by which the crypt key is stored (remote file name or GUID).
 * @param error [OUT] Pointer to the error string.
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_unregister(char *id, char **error);


#ifdef __cplusplus
}
#endif

#endif /* GLITE_DATA_EDS_SIMPLE_H */
