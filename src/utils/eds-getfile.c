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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <glite/data/hydra/c/eds-simple.h>
#include <gfal_api.h>

#define PROGNAME     "glite-eds-get"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE     10000000
#define GFAL_LFN_LENGTH		  256

#define TOOL_USER_VERBOSE   "__GLITE_EDS_VERBOSE"

#define true	1
#define false	0


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
    char   localfilename [GFAL_LFN_LENGTH];
    char   remotefilename[GFAL_LFN_LENGTH + 7];

    struct timeval abs_start_time;
    struct timeval abs_stop_time;
    struct timezone tz;

    char *id = NULL;
    char *service_endpoint = NULL;
    int silent     = false;

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


    // Check if remote file name format:
    //   Must be lfn://<filename> or guid://<guid>
    // TODO
    // -------------------------------------------------------------------------
    if (strlen(argv[optind]) > GFAL_LFN_LENGTH + 6) {
        TRACE_ERR((stderr, "Remote filename is too long (more than %d chars)!\n", GFAL_LFN_LENGTH + 6));
	return -1;
    }
    strcpy(remotefilename,argv[optind]);

    // Copy local file name
    // -------------------------------------------------------------------------
    if (strlen(argv[optind + 1]) > GFAL_LFN_LENGTH - 1) {
        TRACE_ERR((stderr, "Local filename is too long (more than %d chars)!\n", GFAL_LFN_LENGTH - 1));
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
    TRACE_LOG((stdout,"Open remote file"));
    int fh;
	if ((fh = gfal_open (remotefilename,O_RDONLY, 0)) < 0) {
		        TRACE_ERR((stderr,"Cannot Open Remote File %s. Error is %s (code: %d)\"\n",remotefilename,strerror(errno),errno));
		        return -1;
	}
	

    // Initialize eds library
    // -------------------------------------------------------------------------
    TRACE_LOG((stdout,"Initialize EDS library"));
    char *error;
    EVP_CIPHER_CTX *dctx;
    char errbuf[256];

    if(NULL == id) {

   	  if((id = guidfromlfn(remotefilename, errbuf, sizeof(errbuf))) == NULL){
	  	TRACE_ERR((stderr,"Cannot get guid for remote File %s. Error is %s (code: %d)\"\n",remotefilename,errbuf,errno));
	  	gfal_close(fh);
	    return (-1);
   	  }
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

    
    TRACE_LOG((stdout,"Start File Transfer"));
    
    // Get File Status
    // -------------------------------------------------------------------------
    struct stat statbuf;
    int result;
	if ((result = gfal_stat (remotefilename, &statbuf)) < 0) {
        TRACE_ERR((stderr,"Cannot Get Remote File Stat. Error is \"%s (code: %d)\"\n",strerror(errno), errno));
        gfal_close(fh);
        return -1;
    } 
    
    long long size = statbuf.st_size;

    // Open local file
    // -------------------------------------------------------------------------
    int fdump = open(localfilename,O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fdump < 0) {
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr,"Cannot Create Local File %s. Error is \"%s (code: %d)\"\n",localfilename, error_msg, errno));
        gfal_close(fh);
        return -1;
    }

    int counter;
    long long bytesread = 0;
    
    // Read Remote File
    // -------------------------------------------------------------------------
    while (bytesread < size) {
        int nread =  gfal_read(fh,buffer,TRANSFERBLOCKSIZE);
        if (nread <= 0) {
            
            TRACE_ERR((stderr,"\nFatal error during remote read. Error is (code: %d)\"\n",nread));
            TRACE_ERR((stderr,"Transfer Finished after %d/%d bytes!\n",bytesread,size));
            close(fdump);
            gfal_close(fh);
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
            gfal_close(fh);
            return -1;
        }
        bytesread += nread;
      
        // Print Progress Bar
        // ---------------------------------------------------------------------
        if(!silent) {
            char sbasename[1024];
            TRACE_LOG((stdout,"[%s] Total %.02f MB\t|",PROGNAME,(float)size/1024/1024));
            int  l;
            for (l=0; l< 20;l++) {
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
        gfal_close(fh);
	return -1;
      }
    if (final_buf_size)
	write(fdump, final_buf, final_buf_size);
    

    // Close Local File
    // -------------------------------------------------------------------------
    close(fdump);

    // Close Remote File
    // -------------------------------------------------------------------------
    result = gfal_close(fh);
    if(0 != result){
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr,"WARNING: Error in Closing Remote File. Error is \"%s (code: %d)\"\n",error_msg, result));
    }
    
    TRACE_LOG((stdout,"File Transfer Completed"));

    gettimeofday (&abs_stop_time, &tz);
    float abs_time=((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec) *1000 + (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));
    
    TRACE_LOG((stdout, "\nTransfer Completed:\n\n"));
    TRACE_LOG((stdout, "  LFN                     : %s  \n", remotefilename));
    TRACE_LOG((stdout, "  GUID                    : %s  \n", id));
    
    char **replicas;
    char **p;
    if((replicas = surlsfromguid(id, errbuf, sizeof(errbuf))) != NULL) {
        for(p = replicas; *p != NULL; p++) {
        	TRACE_LOG((stdout, "  SURL                    : %s  \n", *p));
        }
    }
    TRACE_LOG((stdout, "  Data Written [bytes]    : %lld\n", bytesread));
    if (abs_time!=0)
    {
	TRACE_LOG((stdout, "  Eff.Transfer Rate[Mb/s] : %f  \n",
		   bytesread / abs_time / 1000.0));
    }
    TRACE_LOG((stdout, "\n"));

    // Shut down encryption
    // -------------------------------------------------------------------------
    glite_eds_finalize(dctx, &error);
   
    return 0;
}
