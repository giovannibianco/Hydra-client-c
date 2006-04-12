/*
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  gLite Encrypted Data Storage register client
 *
 *  Authors: 
 *      Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *      Akos Frohner <Akos.Frohner@cern.ch>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <glite/data/hydra/c/eds-simple.h>

#define PROGNAME     "glite-eds-decrypt"
#define PROGAUTHOR   "(C) EGEE"
#define TOOL_USER_VERBOSE   "__GLITE_EDS_VERBOSE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

static void print_usage_and_die(FILE * out){
    fprintf(out, "\n");
    fprintf(out, "<%s> Version %s by %s\n", PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
    fprintf(out, "usage: %s <ID> <input_filename> <output_filename>\n", PROGNAME);
    fprintf(out, " ");
    fprintf(out, " Decrypt a file locally with the key that is stored for a given ID. \n");
    fprintf(out, " ID             : The remote ID (lfn or GUID) of the key \n");
    fprintf(out, " input_filename : The encrypted file to be decrypted \n");
    fprintf(out, " output_filename: The decrypted file which is written \n");
    fprintf(out, " Optional flags:\n");
    fprintf(out, "  -v      : verbose mode\n");
    fprintf(out, "  -q      : quiet mode\n");
    fprintf(out, "  -s URL  : the Hydra KeyStore to talk to\n");
    fprintf(out, "  -h      : print this screen\n");
    if(out == stdout){
        exit(0);
    }
    exit(-1);
}

int main(int argc, char **argv)
{
    int flag;
    char *in, *id, *out;
    char *service_endpoint = NULL;
    int silent = 0; // false

    while ((flag = getopt(argc, argv, "qhvs:")) != -1) {
        switch (flag) {
            case 'q':
                silent = 1; // true
                unsetenv(TOOL_USER_VERBOSE);
                break;
            case 's':
                service_endpoint = strdup(optarg);
                break;
            case 'h':
                print_usage_and_die(stdout);
                break;
            case 'v':
                setenv(TOOL_USER_VERBOSE, "YES", 1);
                silent = 0; // false
                break;
            default:
                print_usage_and_die(stderr);
                break;
        }
    }
    
    if (argc != (optind+3)) {
        print_usage_and_die(stderr);
    }
    
    id = argv[optind++]; in = argv[optind++]; out = argv[optind++];
    
    // Initialize eds library
    // -------------------------------------------------------------------------
    char *error;
    EVP_CIPHER_CTX *dctx;

    if (NULL == (dctx = glite_eds_decrypt_init(id, &error)))
    {
        TRACE_ERR((stderr, "Error during glite_eds_decrypt_init: %s\n", error));
        free(error);
        return -1;
    }


    // Open input file
    // -------------------------------------------------------------------------
    int in_fd = open(in, O_RDONLY);
    if (in_fd < 0) {
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr, "Cannot Open Local Input File %s. "
                    "Error is \"%s (code: %d)\"\n", in, error_msg, errno));
        return -1;
    }

    // Open output file
    // -------------------------------------------------------------------------
    int out_fd = open(out, O_WRONLY|O_CREAT, 0640);
    if (out_fd < 0) {
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr, "Cannot Open Local Output File %s. "
                    "Error is \"%s (code: %d)\"\n", out, error_msg, errno));
        return -1;
    }


    // Do decryption
    // -------------------------------------------------------------------------
    const int in_buf_size = 65536;
    int in_read;
    char *in_buf = (char *)malloc(in_buf_size);
    in_read = read(in_fd, in_buf, in_buf_size);
    while (in_read) {
        if (-1 == in_read)
        {
            const char * error_msg = strerror(errno);
            TRACE_ERR((stderr, "\nFatal error during output write. "
                        "Error is \"%s (code: %d)\"\n", error_msg, errno));
            close(in_fd); close(out_fd);
            return -1;
        }
        int enc_buffer_size;
        char *enc_buffer;
        if (glite_eds_decrypt_block(dctx, in_buf, in_read, &enc_buffer, &enc_buffer_size, &error))
        {
            TRACE_ERR((stderr, "Error during glite_eds_decrypt_block: %s\n",
                       error));
            free(error);
            close(in_fd); close(out_fd);
            return -1;
        }
        
        int out_write = write(out_fd, enc_buffer, enc_buffer_size);
        free(enc_buffer);
        if (out_write != enc_buffer_size) {
            const char * error_msg = strerror(errno);
            TRACE_ERR((stderr, "\nFatal error during output write. "
                        "Error is \"%s (code: %d)\"\n", error_msg, errno));
            close(in_fd); close(out_fd);
            return -1;
        }
        
        in_read = read(in_fd, in_buf, in_buf_size);
    }
    
    int final_buf_size;
    char *final_buf;
    if (glite_eds_decrypt_final(dctx, &final_buf, &final_buf_size, &error))
    {
        TRACE_ERR((stderr, "Error during glite_eds_decrypt_final: %s\n",
                 error));
        free(error);
        close(in_fd); close(out_fd);
        return -1;
    }
    write(out_fd, final_buf, final_buf_size);
    
    if (!silent){
        TRACE_LOG((stdout,"\n"));
    }
    
    free(in_buf);
    
    // Close Local input and output File
    // -------------------------------------------------------------------------
    close(in_fd); close(out_fd);
    
    // Shut down decryption
    // -------------------------------------------------------------------------
    glite_eds_finalize(dctx, &error);
    
    return 0;
}
