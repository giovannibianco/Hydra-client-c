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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <glite/data/hydra/c/eds-simple.h>
#include <glite/data/io/client/ioclient.h>

#define PROGNAME     "glite-eds-register"
#define PROGVERSION  "1.1"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

void print_usage_and_die(FILE * out){
    fprintf(out, "\n");
    fprintf(out, "<%s> Version %s by %s\n", PROGNAME, PROGVERSION, PROGAUTHOR);
    fprintf(out, "usage: %s <localfilename> <remotefilename> <SURL/GUID> <outfilename>\n", PROGNAME);
    fprintf(out, " Optional parameters:\n");
    fprintf(out, "  -v : verbose mode\n");
    fprintf(out, "  -q : quiet mode\n");
    fprintf(out, "  -c : cipher name to use\n");
    fprintf(out, "  -k : key size to use in bits\n");
    fprintf(out, "  -h : print this screen\n");
    if(out == stdout){
        exit(0);
    }
    exit(-1);
}

int main(int argc, char **argv)
{
    int flag, key_size = 0;
    char *in, *remote, *id, *out, *cipher = NULL;
    bool silent = false;

    while ((flag = getopt(argc, argv, "qhvc:k:")) != -1) {
	switch (flag) {
	    case 'q':
		silent = true;
		unsetenv(TOOL_USER_VERBOSE);
		break;
	    case 'h':
		print_usage_and_die(stdout);
		break;
	    case 'v':
		setenv(TOOL_USER_VERBOSE, "YES", 1);
		silent = false;
		break;
	    case 'c':
		cipher = strdup(optarg);
		break;
	    case 'k':
		if (1 != sscanf(optarg, "%d", &key_size))
		{
		    TRACE_ERR((stderr, "Parsing key size failed!"));
		}
		break;
	    default:
		print_usage_and_die(stderr);
		break;
	}
    }
    
    if (argc != (optind+4)) {
	print_usage_and_die(stderr);
    }
    
    in = argv[optind++]; remote = argv[optind++]; id = argv[optind++]; out = argv[optind++];
    
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
    
    // Initialize eds library
    // -------------------------------------------------------------------------
    char *error;
    EVP_CIPHER_CTX *ectx;
    
    if (NULL == (ectx = glite_eds_register_encrypt_init(remote, id,
							cipher, key_size, &error)))
    {
	TRACE_ERR((stderr, "Error during glite_eds_register_encrypt_init: %s\n",
		   error));
	free(error);
	if (glite_eds_unregister(remote, &error))
	{
	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
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
	    if (glite_eds_unregister(remote, &error))
	    {
		TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
			   error));
		free(error);
	    }
	    close(in_fd); close(out_fd);
	    return -1;
	}
	int enc_buffer_size;
	char *enc_buffer;
	if (glite_eds_encrypt_block(ectx, in_buf, in_read, &enc_buffer, &enc_buffer_size, &error))
	{
	    TRACE_ERR((stderr, "Error during glite_eds_encrypt_block: %s\n",
		       error));
	    free(error);
	    if (glite_eds_unregister(remote, &error))
	    {
		TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
			   error));
		free(error);
	    }
	    close(in_fd); close(out_fd);
	    return -1;
	}
	
	int out_write = write(out_fd, enc_buffer, enc_buffer_size);
	free(enc_buffer);
	if (out_write != enc_buffer_size) {
	    const char * error_msg = strerror(errno);
	    TRACE_ERR((stderr,"\nFatal error during output write. Error is \"%s (code: %d)\"\n", error_msg, errno));
	    if (glite_eds_unregister(remote, &error))
	    {
		TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
			   error));
		free(error);
	    }
	    close(in_fd); close(out_fd);
	    return -1;
	}
	
	in_read = read(in_fd, in_buf, in_buf_size);
    }
    
    int final_buf_size;
    char *final_buf;
    if (glite_eds_encrypt_final(ectx, &final_buf, &final_buf_size, &error))
    {
	TRACE_ERR((stderr, "Error during glite_eds_encrypt_final: %s\n",
		 error));
	free(error);
	if (glite_eds_unregister(remote, &error))
	{
	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
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
    glite_eds_finalize(ectx, &error);
    
    return 0;
}
