/*
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Encrypted Data Storage register client
 *
 *  Authors: Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <glite/data/hydra/c/eds-simple.h>
#include <glite/data/io/client/ioclient.h>

#define PROGNAME     "glite-eds-encrypt"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

void print_usage_and_die(FILE * out){
    fprintf(out, "\n");
    fprintf(out, "<%s> Version %s by %s\n", PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
    fprintf(out, "usage: %s <remotefilename> <localfilename> <outfilename>\n", PROGNAME);
    fprintf(out, " ");
    fprintf(out, " Encrypt a local file locally with the key that is stored for a given remote filename. \n");
    fprintf(out, " This command will read the encryption keys for the given remotefile, and encrypt the local file. \n\n");
    fprintf(out, " remotefilename: The remote (lfn) path of the file to fetch the keys from \n");
    fprintf(out, " localfilename : The local path to the file to be encrypted \n");
    fprintf(out, " outfilename   : The encrypted file which is written \n");
    fprintf(out, " Optional flags:\n");
    fprintf(out, "  -v      : verbose mode\n");
    fprintf(out, "  -q      : quiet mode\n");
    fprintf(out, "  -s URL  : the IO server to talk to\n");
    fprintf(out, "  -h      : print this screen\n");
    if(out == stdout){
        exit(0);
    }
    exit(-1);
}

int main(int argc, char **argv)
{
    int flag, key_size = 0;
    char *in, *remote, *id, *out, *cipher = NULL;
    char *service_endpoint = NULL;
    bool silent = false;

    while ((flag = getopt(argc, argv, "qhv")) != -1) {
	switch (flag) {
	    case 'q':
		silent = true;
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
		silent = false;
		break;
	    default:
		print_usage_and_die(stderr);
		break;
	}
    }
    
    if (argc != (optind+3)) {
	print_usage_and_die(stderr);
    }
    
    remote = argv[optind++]; in = argv[optind++]; out = argv[optind++];
    
    // Initialize the io client
    // -------------------------------------------------------------------------
    int initres = glite_io_initialize(service_endpoint, false);
    if (initres < 0) {
        TRACE_ERR((stderr,"Cannot Initialize!\n"));
        return -1;
    }

    // Open remote file
    // -------------------------------------------------------------------------
    glite_result gl_res;
    glite_handle fh = glite_open(remote,O_RDONLY,0,0,&gl_res);
    if (0 == fh) {
        const char * error_msg = glite_strerror(gl_res);
        TRACE_ERR((stderr,"Cannot Open Remote File %s. Error is \"%s (code: %d)\"\n",remote,error_msg, gl_res));
        return -1;
    }

    // Initialize eds library
    // -------------------------------------------------------------------------
    char *error;
    EVP_CIPHER_CTX *dctx;

    struct glite_stat stat_buf;
    glite_int32 result = glite_fstat(fh, &stat_buf);
    if(0 != result)
	{
	    const char * error_msg = glite_strerror(result);
	    TRACE_ERR((stderr, "Cannot Get Remote File Stat. Error is \"%s (code: "
		       "%d)\"\n",error_msg, result));
	    glite_close(fh);
	    return -1;
	} 

    id = stat_buf.guid;
    glite_close(fh);

    if (NULL == (dctx = glite_eds_decrypt_init(id, &error)))
    {
	TRACE_ERR((stderr, "Error during glite_eds_decrypt_init: %s\n",
	    error));
	free(error);
	return -1;
    }


    // Open input file
    // -------------------------------------------------------------------------
    int in_fd = open(in, O_RDONLY);
    if (in < 0) {
	const char * error_msg = strerror(errno);
	TRACE_ERR((stderr,"Cannot Open Local Input File %s. Error is \"%s (code: %d)\"\n", in, error_msg, errno));
	return -1;
    }

    // Open output file
    // -------------------------------------------------------------------------
    int out_fd = open(out, O_WRONLY|O_CREAT, 0640);
    if (out_fd < 0) {
	const char * error_msg = strerror(errno);
	TRACE_ERR((stderr,"Cannot Open Local Output File %s. Error is \"%s (code: %d)\"\n", out, error_msg, errno));
	return -1;
    }


    // Do encryption
    // -------------------------------------------------------------------------
    const int in_buf_size = 65536;
    int in_read;
    char *in_buf = (char *)malloc(in_buf_size);
    in_read = read(in_fd, in_buf, in_buf_size);
    while (in_read) {
	if (-1 == in_read)
	{
	    const char * error_msg = strerror(errno);
	    TRACE_ERR((stderr,"\nFatal error during output write. Error is \"%s (code: %d)\"\n", error_msg, errno));
	    close(in_fd); close(out_fd);
	    return -1;
	}
	int enc_buffer_size;
	char *enc_buffer;
	if (glite_eds_encrypt_block(dctx, in_buf, in_read, &enc_buffer, &enc_buffer_size, &error))
	{
	    TRACE_ERR((stderr, "Error during glite_eds_encrypt_block: %s\n",
		       error));
	    free(error);
	    close(in_fd); close(out_fd);
	    return -1;
	}
	
	int out_write = write(out_fd, enc_buffer, enc_buffer_size);
	free(enc_buffer);
	if (out_write != enc_buffer_size) {
	    const char * error_msg = strerror(errno);
	    TRACE_ERR((stderr,"\nFatal error during output write. Error is \"%s (code: %d)\"\n", error_msg, errno));
	    close(in_fd); close(out_fd);
	    return -1;
	}
	
	in_read = read(in_fd, in_buf, in_buf_size);
    }
    
    int final_buf_size;
    char *final_buf;
    if (glite_eds_encrypt_final(dctx, &final_buf, &final_buf_size, &error))
    {
	TRACE_ERR((stderr, "Error during glite_eds_encrypt_final: %s\n",
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
    
    // Shut down encryption
    // -------------------------------------------------------------------------
    glite_eds_finalize(dctx, &error);
    
    return 0;
}
