/*
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Encrypted Data Storage get client
 *
 *  Authors: Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#define PROGNAME     "glite-eds-get"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE     10000000

void print_usage_and_die(FILE * out){
    fprintf (out, "\n");
    fprintf (out, "<%s> Version %s by %s\n", PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
    fprintf (out, "usage: %s <remotefilename> <localfilename> [-i <id>]\n", PROGNAME);
    fprintf(out, "  -i <id>        : the ID to use to look up the decryption key of this file "
	    "(defaults to the remotefilename's GUID).\n");
    fprintf (out, " Optional parameters:\n");
    fprintf (out, "  -v      : verbose mode\n");
    fprintf (out, "  -q      : quiet mode\n");
    fprintf (out, "  -s URL  : the IO server to talk to\n");
    fprintf (out, "  -h      : print this screen\n");
    if(out == stdout){
        exit(0);
    }
    exit(-1);
}

int main(int argc, char* argv[])
{
    char   localfilename [GLITE_LFN_LENGTH];
    char   remotefilename[GLITE_LFN_LENGTH + 7];

    struct timeval abs_start_time;
    struct timeval abs_stop_time;
    struct timezone tz;

    char *id = NULL;
    char *service_endpoint = NULL;
    bool silent     = false;

    int flag;
    while ((flag = getopt (argc, argv, "qhvs:i:")) != -1) {
        switch (flag) {
            case 'q':
                silent = true;
		unsetenv(TOOL_USER_VERBOSE);
        	break;
            case 'h':
                print_usage_and_die(stdout);
        	break;
	    case 's':
		service_endpoint = strdup(optarg);
		break;
	    case 'i':
		id = strdup(optarg);
		break;
	    case 'v':
		setenv(TOOL_USER_VERBOSE, "YES", 1);
		silent = false;
		break;
            default:
                print_usage_and_die(stderr);
        	break;
        } // End Switch
    } // End while

    if (argc != (optind+2)) {
        print_usage_and_die(stderr);
    }

    // Get The Logger
    // -------------------------------------------------------------------------
    Category& logger = Category::getInstance("glite_get");

    // Initialize the io client
    // -------------------------------------------------------------------------
    logger.log(Priority::INFO,"Initializing IO");
    int initres = glite_io_initialize(service_endpoint, false);
    if (initres < 0) {
        TRACE_ERR((stderr,"Cannot Initialize!\n"));
        return -1;
    }

    // Check if remote file name format:
    //   Must be lfn://<filename> or guid://<guid>
    // TODO
    // -------------------------------------------------------------------------
    if (strlen(argv[optind]) > GLITE_LFN_LENGTH + 6) {
        TRACE_ERR((stderr, "Remote filename is too long (more than %d chars)!\n", GLITE_LFN_LENGTH + 6));
	return -1;
    }
    strcpy(remotefilename,argv[optind]);

    // Copy local file name
    // -------------------------------------------------------------------------
    if (strlen(argv[optind + 1]) > GLITE_LFN_LENGTH - 1) {
        TRACE_ERR((stderr, "Local filename is too long (more than %d chars)!\n", GLITE_LFN_LENGTH - 1));
	return -1;
    }
    strcpy(localfilename ,argv[optind + 1]);

    char *buffer = (char *)malloc(TRANSFERBLOCKSIZE);
    if (!buffer) {
        TRACE_ERR((stderr, "Failed to allocate transfer buffer of size %d bytes!\n", TRANSFERBLOCKSIZE));
	return -1;
    }

    // Open remote file
    // -------------------------------------------------------------------------
    logger.log(Priority::INFO,"Open remote file");
    glite_result gl_res;
    glite_handle fh = glite_open(remotefilename,O_RDONLY,0,0,&gl_res);
    if (0 == fh) {
        const char * error_msg = glite_strerror(gl_res);
        TRACE_ERR((stderr,"Cannot Open Remote File %s. Error is \"%s (code: %d)\"\n",remotefilename,error_msg, gl_res));
        return -1;
    }

    // Initialize eds library
    // -------------------------------------------------------------------------
    logger.log(Priority::INFO,"Initialize EDS library");
    char *error;
    EVP_CIPHER_CTX *dctx;

    if(NULL == id) {

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
    }

    if (NULL == (dctx = glite_eds_decrypt_init(id, &error)))
    {
	TRACE_ERR((stderr, "Error during glite_eds_decrypt_init: %s\n",
	    error));
	free(error);
	return -1;
    }

    // Start Time    
    // -------------------------------------------------------------------------
    gettimeofday(&abs_start_time,&tz);

    
    logger.log(Priority::INFO,"Start File Transfer");
    
    // Get File Status
    // -------------------------------------------------------------------------
    struct glite_stat stat_buf;
    int result = glite_fstat(fh,&stat_buf);
    if(0 != result){
        const char * error_msg = glite_strerror(result);
        TRACE_ERR((stderr,"Cannot Get Remote File Stat. Error is \"%s (code: %d)\"\n",error_msg, result));
        glite_close(fh);
        return -1;
    } 
    long long size = stat_buf.size;

    // Open local file
    // -------------------------------------------------------------------------
    int fdump = open(localfilename,O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fdump < 0) {
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr,"Cannot Create Local File %s. Error is \"%s (code: %d)\"\n",localfilename, error_msg, errno));
        glite_close(fh);
        return -1;
    }

    int counter;
    long long bytesread = 0;
    
    // Read Remote File
    // -------------------------------------------------------------------------
    while (bytesread < size) {
        int nread =  glite_read(fh,buffer,TRANSFERBLOCKSIZE);
        if (nread <= 0) {
            const char * error_msg = glite_strerror(nread);
            TRACE_ERR((stderr,"\nFatal error during remote read. Error is \"%s (code: %d)\"\n",error_msg, nread));
            TRACE_ERR((stderr,"Transfer Finished after %d/%d bytes!\n",bytesread,size));
            close(fdump);
            glite_close(fh);
            return -1;
        }
	int dec_buffer_size;
	char *dec_buffer;
	if (glite_eds_decrypt_block(dctx, buffer, nread, &dec_buffer, &dec_buffer_size, &error))
	{
	    TRACE_ERR((stderr, "Error during glite_eds_encrypt_block: %s\n",
	        error));
	    free(error);
	}
        int nwrite = write(fdump, dec_buffer, dec_buffer_size);
	free(dec_buffer);
        if (nwrite != dec_buffer_size) {
            const char * error_msg = strerror(errno);
            TRACE_ERR((stderr,"\nFatal error during local write. Error is \"%s (code: %d)\"\n",error_msg, errno));
            TRACE_ERR((stderr,"Transfer Finished after %d/%d bytes!\n",bytesread,size));
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

    // Finalize decryption, write final block
    // -------------------------------------------------------------------------
    int final_buf_size;
    char *final_buf;
    if (glite_eds_decrypt_final(dctx, &final_buf, &final_buf_size, &error))
      {
	TRACE_ERR((stderr, "Error during glite_eds_encrypt_final: %s\n",
		   error));
	free(error);
        close(fdump);
        glite_close(fh);
	return -1;
      }
    if (final_buf_size)
	write(fdump, final_buf, final_buf_size);
    

    // Close Local File
    // -------------------------------------------------------------------------
    close(fdump);

    // Close Remote File
    // -------------------------------------------------------------------------
    result = glite_close(fh);
    if(0 != result){
        const char * error_msg = glite_strerror(result);
        TRACE_ERR((stderr,"WARNING: Error in Closing Remote File. Error is \"%s (code: %d)\"\n",error_msg, result));
    }
    
    logger.log(Priority::INFO,"File Transfer Completed");

    gettimeofday (&abs_stop_time, &tz);
    float abs_time=((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec) *1000 + (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));
    
    TRACE_LOG((stdout,"\nTransfer Completed:\n\n"));
    TRACE_LOG((stdout,"  LFN                     : %s  \n",stat_buf.lfn));
    TRACE_LOG((stdout,"  GUID                    : %s  \n",stat_buf.guid));
    TRACE_LOG((stdout,"  SURL                    : %s  \n",stat_buf.surl));
    TRACE_LOG((stdout,"  Data Written [bytes]    : %lld\n",bytesread));
    if (abs_time!=0) {
    TRACE_LOG((stdout,"  Eff.Transfer Rate[Mb/s] : %f  \n",bytesread/abs_time/1000.0));
    }
    TRACE_LOG((stdout,"\n"));
    
    glite_io_finalize();

    // Shut down encryption
    // -------------------------------------------------------------------------
    glite_eds_finalize(dctx, &error);
   
    return 0;
}
