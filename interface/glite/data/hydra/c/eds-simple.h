/*
 *  Copyright (c) Members of the EGEE Collaboration. 2005.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Data Management - Simple encrypted data storage API
 *
 *  Authors:
 *	Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *
 */

#ifndef GLITE_DATA_EDS_SIMPLE_H
#define GLITE_DATA_EDS_SIMPLE_H

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Register a new file in Hydra: create metadata entries (key/iv/...)
 * 
 * @param lfn The name of the remote file.
 * @param id The SURL or GUID of the remote file.
 * @param cipher The cipher name to use.
 * @param keysize Key size to use in bits.
 * @param [OUT] error Pointer to the error string.
 *
 * @return 0 in case of no error. In other cases *error contains the error
 *  string. The caller is responsible for freeing the allocated error string.
 */
int glite_eds_register(char *lfn, char *id, char *cipher, int keysize,
    char **error);

/**
 * Register a new file in Hydra: create metadata entries (key/iv/...),
 * initalizes encryption context
 * 
 * @param lfn The name of the remote file.
 * @param id The SURL or GUID of the remote file.
 * @param cipher The cipher name to use.
 * @param keysize Key size to use in bits.
 * @param [OUT] error Pointer to the error string.
 *
 * @return Encryption context in case of no error. In other cases NULL is
 *  returned, and *error contains the error string. The caller is responsible
 *  for freeing the allocated error string.
 */
EVP_CIPHER_CTX *glite_eds_register_encrypt_init(char *lfn, char *id,
    char *cipher, int keysize, char **error);

/**
 * Initialize encryption context for a file. Query key/iv/... from
 * metadata catalog
 *
 * @param lfn The name of the remote file.
 * @param [OUT] error Pointer to the error string.
 *
 * @return Encryption context in case of no error. In other cases NULL is
 *  returned, and *error contains the error string. The caller is responsible
 *  for freeing the allocated error string.
 */
EVP_CIPHER_CTX *glite_eds_encrypt_init(char *lfn, char **error); 

/**
 * Initialize decryption context for a file. Query key/iv/... from
 * metadata catalog
 *
 * @param lfn The name of the remote file.
 * @param [OUT] error Pointer to the error string.
 *
 * @return Decryption context in case of no error. In other cases NULL is
 *  returned, and *error contains the error string. The caller is responsible
 *  for freeing the allocated error string.
 */
EVP_CIPHER_CTX *glite_eds_decrypt_init(char *lfn, char **error); 

/**
 * Encrypts a memory block using the encryption context
 * 
 * @param ctx Encryption context 
 * @param mem_in Memory block to encrypt
 * @param mem_in_size Memory block size
 * @param [OUT] mem_out Encrypted memory block's address
 * @param [OUT] mem_out_size Encrypted memory block's size
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
 * @param ctx Encryption context 
 * @param mem_in Memory block to encrypt
 * @param mem_in_size Memory block size
 * @param [OUT] mem_out Encrypted memory block's address
 * @param [OUT] mem_out_size Encrypted memory block's size
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_encrypt_final(EVP_CIPHER_CTX *ectx, char **mem_out, int *mem_out_size, char **error);

/**
 * Decrypts a memory block using the encryption context
 * 
 * @param ctx Decryption context 
 * @param mem_in Memory block to decrypt
 * @param mem_in_size Memory block size
 * @param [OUT] mem_out Decrypted memory block's address
 * @param [OUT] mem_out_size Decrypted memory block's size
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
 * @param ctx Decryption context 
 * @param mem_in Memory block to decrypt
 * @param mem_in_size Memory block size
 * @param [OUT] mem_out Decrypted memory block's address
 * @param [OUT] mem_out_size Decrypted memory block's size
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
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_finalize(EVP_CIPHER_CTX *ctx, char **error);

/**
 * Unregister catalog entries in case of error (key/iv)
 *
 * @param lfn The name of the remote file.
 *
 * @return 0 in case of there was no error. In other cases, *error contains
 *  the error string. The caller is responsible for freeing the allocated string
 *  and the returned memory block in case of success
 */
int glite_eds_unregister(char *lfn, char **error);


#ifdef __cplusplus
}
#endif

#endif /* GLITE_DATA_EDS_SIMPLE_H */
