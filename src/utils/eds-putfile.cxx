/*
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Encrypted Data Storage put client
 *
 *  Authors: Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log4cpp/Category.hh"

#include <glite/data/hydra/c/eds-simple.h>
#include <glite/data/io/client/ioclient.h>

using namespace log4cpp;

#define PROGNAME     "glite-eds-put"
#define PROGVERSION  "1.1"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE 10000000

void print_usage_and_die(FILE * out){
    fprintf(out, "\n");
    fprintf(out, "<%s> Version %s by %s\n", PROGNAME, PROGVERSION, PROGAUTHOR);
    fprintf(out, "usage: %s <localfilename> <remotefilename> [-m <mode>]\n",
	    PROGNAME);
    fprintf(out, " Optional parameters:\n");
    fprintf(out, "  -m <mode>        : the permission to use for the new file "
	    "(default is 0640).\n");
    fprintf(out, "  -v : verbose mode\n");
    fprintf(out, "  -q : quiet mode\n");
    fprintf(out, "  -c : cipher name to use\n");
    fprintf(out, "  -k : key size to use in bits\n");
    fprintf(out, "  -h : print this screen\n");
    if (out == stdout) {
	exit(0);
    }
    exit(-1);
}

int main(int argc, char **argv)
{
    int flag, key_size = 0, mode = 0640;
    char *in, *remote, *id, *out, *cipher = NULL;
    bool silent = false, progbar = false;
    char localfilename[GLITE_LFN_LENGTH];
    char remotefilename[GLITE_LFN_LENGTH + 7];
    
    struct timeval abs_start_time;
    struct timeval abs_stop_time;
    struct timezone tz;

    while ((flag = getopt (argc, argv, "m:qhvc:k:")) != -1) {
        switch (flag) {
            case 'm':
                sscanf(optarg,"%o",&mode);
        	break;
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
        } // End Switch
    } // End while
    
    if (argc != (optind+2)) {
        print_usage_and_die(stderr);
    }
    
    if ((mode < 0001) || (mode > 0777)) {
        TRACE_ERR((stderr,"Invalid permission specified. The value should be in"
		   " the range [0001,0777]\n"));
        print_usage_and_die(stderr);
    }
    
    // Initialize the client
    // -------------------------------------------------------------------------
    int initres = glite_io_initialize(NULL, false);
    if (initres < 0) {
        TRACE_ERR((stderr, "Cannot Initialize!\n"));
        return -1;
    }       
    
    // Copy Local file name
    // -------------------------------------------------------------------------
    if (strlen(argv[optind]) > GLITE_LFN_LENGTH-1) {
        TRACE_ERR((stderr, "Local filename is too long (more than %d "
		   "chars)!\n", GLITE_LFN_LENGTH - 1));
	return -1;
    }
    strcpy(localfilename ,argv[optind]);
    
    // Check if remote file name format: 
    //   Must be lfn://<filename> or guid://<guid>
    // TODO
    // -------------------------------------------------------------------------
    if (strlen(argv[optind+1]) > GLITE_LFN_LENGTH + 6) {
        TRACE_ERR((stderr, "Remote filename is too long (more than %d "
		   "chars)!\n", GLITE_LFN_LENGTH + 6));
	return -1;
    }
    strcpy(remotefilename, argv[optind + 1]);
    
    char *buffer = (char *)malloc(TRANSFERBLOCKSIZE);
    if (!buffer) {
        TRACE_ERR((stderr, "Failed to allocate transfer buffer of size %d "
		   "bytes!\n", TRANSFERBLOCKSIZE));
	return -1;
    }
    
    gettimeofday(&abs_start_time,&tz);
    
    // Open local file
    // -------------------------------------------------------------------------
    int fdump = open(localfilename, O_RDONLY);
    if (fdump < 0) {
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr, "Cannot Open Local File %s. Error is \"%s (code: "
		   "%d)\"\n", localfilename, error_msg, errno));
        return -1;
    }
    
    // Get local file size
    // -------------------------------------------------------------------------
    long long size;
    struct stat st_buf;
    int result = fstat(fdump,&st_buf);
    if (result < 0) {
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr, "Fatal error during fstat on local file %s. Error is"
		   " \"%s (code: %d)\"\n",localfilename, error_msg, errno));
        close(fdump);
        return -1;
    }
    else {
        size = st_buf.st_size;
    }
    
    // Initialize eds library
    // -------------------------------------------------------------------------
    char *error;
    EVP_CIPHER_CTX *ectx;
    
    if (NULL == (ectx = glite_eds_register_encrypt_init(remotefilename, NULL,
							cipher, key_size,
							&error)))
    {
	TRACE_ERR((stderr, "Error during glite_eds_register_encrypt_init: %s\n",
		   error));
	free(error);
	if (glite_eds_unregister(remotefilename, &error))
	{
    	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
	return -1;
    }
    
    // Open remote file
    // -------------------------------------------------------------------------
    glite_result gl_res;
    glite_handle fh = glite_creat(remotefilename, mode, size, &gl_res);
    if (0 == fh) {
    	const char * error_msg = glite_strerror(gl_res);
        TRACE_ERR((stderr,"Cannot Create Remote File %s. Error is \"%s (code: "
		   "%d)\"\n",remotefilename,error_msg, gl_res));
	if (glite_eds_unregister(remotefilename, &error))
	{
    	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
        close(fdump);
        return -1;
    }
    // Get The Logger
    // -------------------------------------------------------------------------
    Category& logger = Category::getInstance(PROGNAME);
    
    logger.log(Priority::INFO, "Start File Transfer");
    
    int counter;
    long long bytesread = 0;
    
    // Read Local File
    // -------------------------------------------------------------------------
    while (bytesread < size) {
        int nread =  read(fdump,buffer,TRANSFERBLOCKSIZE);
        if (nread <= 0) {
            const char * error_msg = strerror(errno);
            TRACE_ERR((stderr,"\nFatal error during local read. Error is \"%s "
		       "(code: %d)\"\n", error_msg, errno));
            TRACE_ERR((stderr,"Transfer Finished after %d/%d bytes!\n",
		       bytesread, size));
	    if (glite_eds_unregister(remotefilename, &error))
	    {
    		TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
			   error));
		free(error);
	    }
            close(fdump);
            glite_close(fh);
            return -1;
        }
	
	int enc_buffer_size;
	char *enc_buffer;
	if (glite_eds_encrypt_block(ectx, buffer, nread, &enc_buffer, 
				    &enc_buffer_size, &error))
	{
	    TRACE_ERR((stderr, "Error during glite_eds_encrypt_block: %s\n",
		       error));
	    free(error);
	    if (glite_eds_unregister(remotefilename, &error))
	    {
    		TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
			   error));
		free(error);
	    }
            close(fdump);
            glite_close(fh);
	    return -1;
	}
        
        int nwrite = glite_write(fh, enc_buffer, enc_buffer_size);
	free(enc_buffer);
        if (nwrite != enc_buffer_size) {
            const char * error_msg = glite_strerror(nwrite);
            TRACE_ERR((stderr, "\nFatal error during remote write. Error is "
		       "\"%s (code: %d)\"\n",error_msg, nwrite));
            TRACE_ERR((stderr,"Transfer Finished after %d/%d bytes!\n",
		       bytesread, size));
	    if (glite_eds_unregister(remotefilename, &error))
	    {
    		TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
			   error));
		free(error);
	    }
            close(fdump);
            glite_close(fh);
            return -1;
        }
        bytesread += nread;
	
        // Print Progress Bar
        // ---------------------------------------------------------------------
        if(!silent) {
            char sbasename[1024];
            TRACE_LOG((stdout,"[%s] Total %.02f MB\t|",PROGNAME,(float)size/1024/1024));
            for (int l=0; l< 20;l++) {
                if (l< ( (int)(20.0*bytesread/size))){
                    TRACE_LOG((stdout,"="));
                }
                if (l==( (int)(20.0*bytesread/size))) {
                    TRACE_LOG((stdout,">"));
                }
                if (l> ( (int)(20.0*bytesread/size))){
                    TRACE_LOG((stdout,"."));
                }
            }
	    
            gettimeofday (&abs_stop_time, &tz);
            float abs_time=((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec) *1000 + (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));
            TRACE_LOG((stdout,"| %.02f \% [%.01f Mb/s]\r",100.0*bytesread/size,bytesread/abs_time/1000.0));
            fflush(stdout);
        }  // End Progress Bar
    } // End While
    
    if (!silent){
        TRACE_LOG((stdout,"\n"));
    }
    
    free(buffer);
    
    // Write out final block
    // -------------------------------------------------------------------------
    int final_buf_size;
    char *final_buf;
    if (glite_eds_encrypt_final(ectx, &final_buf, &final_buf_size, &error))
    {
	TRACE_ERR((stderr, "Error during glite_eds_encrypt_final: %s\n",
		   error));
	free(error);
	if (glite_eds_unregister(remotefilename, &error))
	{
	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
        close(fdump);
        glite_close(fh);
	return -1;
    }
    glite_write(fh, final_buf, final_buf_size);
    free(final_buf);
    
    
    // Close Local File
    // -------------------------------------------------------------------------
    close(fdump);
    
    gettimeofday(&abs_stop_time, &tz);
    float abs_time = ((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec)*1000
			      + (abs_stop_time.tv_usec - abs_start_time.tv_usec)
			      / 1000));
    
    // Get File Status
    // -------------------------------------------------------------------------
    struct glite_stat stat_buf;
    result = glite_fstat(fh, &stat_buf);
    if(0 != result)
    {
        const char * error_msg = glite_strerror(result);
        TRACE_ERR((stderr, "Cannot Get Remote File Stat. Error is \"%s (code: "
		   "%d)\"\n",error_msg, result));
	if (glite_eds_unregister(remotefilename, &error))
	{
    	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
        glite_close(fh);
        return -1;
    } 
    
    // Close Remote File
    // -------------------------------------------------------------------------
    result = glite_close(fh);
    if(0 != result){
        const char * error_msg = glite_strerror(result);
        TRACE_ERR((stderr,"WARNING: Error in Closing Remote File. Error is "
		   "\"%s (code: %d)\"\n", error_msg, result));
	if (glite_eds_unregister(remotefilename, &error))
	{
    	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
    }
    
    logger.log(Priority::INFO, "File Transfer Completed");
    
    TRACE_LOG((stdout, "\nTransfer Completed:\n\n"));
    TRACE_LOG((stdout, "  LFN                     : %s  \n", stat_buf.lfn));
    TRACE_LOG((stdout, "  GUID                    : %s  \n", stat_buf.guid));
    TRACE_LOG((stdout, "  SURL                    : %s  \n", stat_buf.surl));
    TRACE_LOG((stdout, "  Data Written [bytes]    : %lld\n", bytesread));
    if (abs_time!=0)
    {
	TRACE_LOG((stdout, "  Eff.Transfer Rate[Mb/s] : %f  \n",
		   bytesread / abs_time / 1000.0));
    }
    TRACE_LOG((stdout, "\n"));
    glite_io_finalize();
    
    // Shut down encryption
    // -------------------------------------------------------------------------
    glite_eds_finalize(ectx, &error);
    
    return 0;
}
