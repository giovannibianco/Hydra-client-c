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


#define PROGNAME     "glite-eds-put"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE 10000000
#define GFAL_LFN_LENGTH		  256

#define true	1
#define false	0


#define TOOL_USER_VERBOSE   "__GLITE_EDS_VERBOSE"

static void print_usage_and_die(FILE * out){
    fprintf(out, "\n");
    fprintf(out, "usage: %s <localfilename> <remotefilename> [-i <id>]\n",
	    PROGNAME);
    fprintf(out, " Optional parameters:\n");
    fprintf(out, "  -i <id>        : the ID to use to look up the decryption key of this file "
	    "(defaults to the remotefilename's GUID).\n");
    fprintf(out, "  -c name : cipher name to use\n");
    fprintf(out, "  -k n    : key size to use in bits\n");
    fprintf(out, "  -u      : don't actually encrypt the data, just do the key gen/registration\n");
    fprintf(out, "            this is useful for some special setups where the SE crypts by itself\n");
    fprintf(out, "  -h      : print this screen\n");
    fprintf(out, "  -q      : quiet mode\n");
    fprintf(out, "  -v      : verbose mode\n");
    fprintf(out, "  -V      : print version and exit\n");
    if (out == stdout) {
	exit(0);
    }
    exit(-1);
}

int main(int argc, char **argv)
{
    int flag, key_size = 0;
    char *cipher = NULL;
    int silent = false, reg_only = false;
    char localfilename[GFAL_LFN_LENGTH];
    char remotefilename[GFAL_LFN_LENGTH + 7];
    
    struct timeval abs_start_time;
    struct timeval abs_stop_time;
    struct timezone tz;
    char *id = NULL;

    while ((flag = getopt (argc, argv, "qhvVuc:k:i:")) != -1) {
        switch (flag) {
		case 'q':
			silent = true;
			unsetenv(TOOL_USER_VERBOSE);
        	break;
		case 'u':
			reg_only = true;
			unsetenv(TOOL_USER_VERBOSE);
        	break;
		case 'h':
			print_usage_and_die(stdout);
        	break;
		case 'i':
			id = strdup(optarg);
			break;
	    case 'v':
			setenv(TOOL_USER_VERBOSE, "YES", 1);
			silent = false;
			break;
	    case 'V':
			fprintf(stdout, "<%s> Version %s by %s\n",
				PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
			exit(0);
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
    
    // Copy Local file name
    // -------------------------------------------------------------------------
    if (strlen(argv[optind]) > GFAL_LFN_LENGTH-1) {
        TRACE_ERR((stderr, "Local filename is too long (more than %d "
		   "chars)!\n", GFAL_LFN_LENGTH - 1));
	return -1;
    }
    strcpy(localfilename ,argv[optind]);
    
    // 
    // -------------------------------------------------------------------------
    if (strlen(argv[optind+1]) > GFAL_LFN_LENGTH + 6) {
        TRACE_ERR((stderr, "Remote filename is too long (more than %d "
		   "chars)!\n", GFAL_LFN_LENGTH + 6));
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
    
    // Open remote file
    // -------------------------------------------------------------------------
    int fh;
	if ((fh = gfal_open (remotefilename, O_WRONLY|O_CREAT, 0644)) < 0) {
    	const char * error_msg = strerror(errno);
        TRACE_ERR((stderr,"Cannot Create Remote File %s. Error is \"%s (code: "
		   "%d)\"\n",remotefilename,error_msg, errno));
        close(fdump);
        return -1;
	}


    // Initialize eds library
    // -------------------------------------------------------------------------
    char *error;
    EVP_CIPHER_CTX *ectx;
    char errbuf[256];

    if(NULL == id) {

	if((id = guidfromlfn(remotefilename, errbuf, sizeof(errbuf))) == NULL){
		TRACE_ERR((stderr,"Cannot get guid for remote File %s. Error is %s (code: %d)\"\n",remotefilename,errbuf,errno));
		gfal_close(fh);
		return (-1);
	  }
    }
    
    if (NULL == (ectx = glite_eds_register_encrypt_init(id,
							cipher, key_size,
							&error)))
    {
	TRACE_ERR((stderr, "Error during glite_eds_register_encrypt_init: %s\n",
		   error));
	free(error);
	gfal_close(fh);
	if(gfal_unlink(remotefilename)<0) {
		TRACE_ERR((stderr,"Warning: cannot unlink remote file %s. Error is %s (code: %d)\"\n",remotefilename,strerror(errno),errno));
	}
		
	if (glite_eds_unregister(id, &error))
	{
    	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
	return -1;
    }
    
    // Get The Logger
    // -------------------------------------------------------------------------
    
    long long bytesread = 0;
    
    // Read Local File
    // -------------------------------------------------------------------------
    while (bytesread < size) {
        int nread =  read(fdump,buffer,TRANSFERBLOCKSIZE);
        if (nread <= 0) {
            const char * error_msg = strerror(errno);
            TRACE_ERR((stderr,"\nFatal error during local read. Error is \"%s "
		       "(code: %d)\"\n", error_msg, errno));
            TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes!\n",
		       bytesread, size));
	    if (glite_eds_unregister(id, &error))
	    {
    		TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
			   error));
		free(error);
	    }
            close(fdump);
            gfal_close(fh);
        	if(gfal_unlink(remotefilename)<0) {
        		TRACE_ERR((stderr,"Warning: cannot unlink remote file %s. Error is %s (code: %d)\"\n",remotefilename,strerror(errno),errno));
        	}

            return -1;
        }

	if(reg_only) {  // don't actually do the encryption

	    int nwrite = gfal_write(fh, buffer, nread);
	    if (nwrite != nread) {
		const char * error_msg = strerror(errno);
		TRACE_ERR((stderr,"\nFatal error during remote write. Error is \"%s (code: %d)\"\n",error_msg, errno));
		TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes!\n",bytesread,size));
		close(fdump);
		gfal_close(fh);
		return -1;
	    }
	    bytesread += nread;


	} else {        // do do the encryption
	    int enc_buffer_size;
	    char *enc_buffer;
	    if (glite_eds_encrypt_block(ectx, buffer, nread, &enc_buffer, 
					&enc_buffer_size, &error))
		{
		    TRACE_ERR((stderr, "Error during glite_eds_encrypt_block: %s\n",
			       error));
		    free(error);
		    if (glite_eds_unregister(id, &error))
			{
			    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
				       error));
			    free(error);
			}
		    close(fdump);
		    gfal_close(fh);
			if(gfal_unlink(remotefilename)<0) {
				TRACE_ERR((stderr,"Warning: cannot unlink remote file %s. Error is %s (code: %d)\"\n",remotefilename,strerror(errno),errno));
			}

		    return -1;
		}
        
	    int nwrite = gfal_write(fh, enc_buffer, enc_buffer_size);
	    free(enc_buffer);
	    if (nwrite != enc_buffer_size) {
		const char * error_msg = strerror(errno);
		TRACE_ERR((stderr, "\nFatal error during remote write. Error is "
			   "\"%s (code: %d)\"\n",error_msg, errno));
		TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes!\n",
			   bytesread, size));
		if (glite_eds_unregister(id, &error))
		    {
			TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
				   error));
			free(error);
		    }
		close(fdump);
		gfal_close(fh);
		if(gfal_unlink(remotefilename)<0) {
			TRACE_ERR((stderr,"Warning: cannot unlink remote file %s. Error is %s (code: %d)\"\n",remotefilename,strerror(errno),errno));
		}

		return -1;
	    }
	    bytesread += nread;
	}

        // Print Progress Bar
        // ---------------------------------------------------------------------
        if(!silent) {
            TRACE_LOG((stdout,"[%s] Total %.02f MB\t|",PROGNAME,(float)size/1024/1024));
            int l;
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
            TRACE_LOG((stdout,"| %.02f %% [%.01f Mb/s]\r",
			100.0*(float)bytesread/(float)size,
			(float)bytesread/abs_time/1000.0));
            fflush(stdout);
        }  // End Progress Bar
    } // End While
    
    if (!silent){
        TRACE_LOG((stdout,"\n"));
    }
    
    free(buffer);


    if(!reg_only) {
    
	// Write out final block
	// -------------------------------------------------------------------------
	int final_buf_size;
	char *final_buf;
	if (glite_eds_encrypt_final(ectx, &final_buf, &final_buf_size, &error))
	    {
		TRACE_ERR((stderr, "Error during glite_eds_encrypt_final: %s\n",
			   error));
		free(error);
		if (glite_eds_unregister(id, &error))
		    {
			TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
				   error));
			free(error);
		    }
		close(fdump);
		gfal_close(fh);
		if(gfal_unlink(remotefilename)<0) {
			TRACE_ERR((stderr,"Warning: cannot unlink remote file %s. Error is %s (code: %d)\"\n",remotefilename,strerror(errno),errno));
		}

		return -1;
	    }
	gfal_write(fh, final_buf, final_buf_size);
	free(final_buf);
    }

    
    // Close Local File
    // -------------------------------------------------------------------------
    close(fdump);
    
    gettimeofday(&abs_stop_time, &tz);
    float abs_time = ((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec)*1000
			      + (abs_stop_time.tv_usec - abs_start_time.tv_usec)
			      / 1000));
    
    // Get File Status
    // -------------------------------------------------------------------------
    struct stat statbuf;
    result = gfal_stat(remotefilename,&statbuf);
    if(result < 0)
    {
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr, "Cannot Get Remote File Stat. Error is \"%s (code: "
		   "%d)\"\n",error_msg, result));
	if (glite_eds_unregister(id, &error))
	{
    	    TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n",
		       error));
	    free(error);
	}
    gfal_close(fh);
	if(gfal_unlink(remotefilename)<0) {
			TRACE_ERR((stderr,"Warning: cannot unlink remote file %s. Error is %s (code: %d)\"\n",remotefilename,strerror(errno),errno));
		}

        return -1;
    }
    
    // Close Remote File
    // -------------------------------------------------------------------------
    result = gfal_close(fh);
    if(0 != result){
        const char * error_msg = strerror(errno);
        TRACE_ERR((stderr,"WARNING: Error in Closing Remote File. Error is "
		   "\"%s (code: %d)\"\n", error_msg, errno));
    }
    

    TRACE_LOG((stdout,"File Transfer Completed"));
    
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
    glite_eds_finalize(ectx, &error);
    
    return 0;
}
