/*
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  GLite Encrypted Data Storage rm client
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
#include <glite/data/io/client/ioerrors.h>

using namespace log4cpp;

#define PROGNAME     "glite-eds-rm"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

void print_usage_and_die(FILE * out){
    fprintf (out, "\n");
    fprintf (out, "<%s> Version %s by %s\n", PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
    fprintf (out, "usage: %s <remotefilename> [-i <id>]\n", PROGNAME);
    fprintf (out, "  -i <id>        : the ID to use to look up the decryption key of this file "
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

int main(int argc, char* argv[]) {

    char   remotefilename[GLITE_LFN_LENGTH + 7];

    struct timeval abs_start_time;
    struct timeval abs_stop_time;
    struct timezone tz;

    char *id = NULL;
    char *service_endpoint = NULL;
    bool silent     = false;
    
    int flag;
    while ((flag = getopt (argc, argv, "qhvi:s:")) != -1) {
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
                silent = false;
                setenv(TOOL_USER_VERBOSE, "YES", 1);
                break;
            default:
                print_usage_and_die(stderr);
            break;
        } // End Switch
    } // End while

    if (argc != (optind+1)) {
        print_usage_and_die(stderr);
    }

    // Initialize the client
    // -------------------------------------------------------------------------
    int initres = glite_io_initialize(service_endpoint, false);
    if (initres < 0) {
        TRACE_ERR((stderr, "Cannot Initialize!\n"));
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

    gettimeofday(&abs_start_time,&tz);


    if(NULL == id) {
        // Open remote file
        // -------------------------------------------------------------------------
        glite_result gl_res;
        glite_handle fh = glite_open(remotefilename,O_RDONLY,0,0,&gl_res);
        if (0 == fh) {
            const char * error_msg = glite_strerror(gl_res);
            TRACE_ERR((stderr,"Cannot Open Remote File %s. Error is \"%s (code: %d)\"\n",remotefilename,error_msg, gl_res));
            return -1;
        }

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

    // Unlink entries in Hydra
    // -------------------------------------------------------------------------
    char *error;
    if (glite_eds_unregister(id, &error))
    {
        TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n", error));
        free(error);
    }
    return -1;

    // Unlink remote file
    // -------------------------------------------------------------------------
    glite_result gl_res = glite_unlink(remotefilename);
    if (GLITE_IO_SUCCESS != gl_res) {
        const char * error_msg = glite_strerror(gl_res);
        TRACE_ERR((stderr, "Cannot Unlink Remote File %s. Error is \"%s (code: %d)\"\n",
            remotefilename, error_msg, gl_res));
        return -1;
    }
    gettimeofday (&abs_stop_time, &tz);
    float abs_time= ((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec) *1000 
        + (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));
        
    TRACE_LOG((stdout,"\nUnlink Completed:\n\n"));
    TRACE_LOG((stdout,"  File          : %s  \n",remotefilename));
    if (abs_time != 0) {
        TRACE_LOG((stdout,"  Time [s]      : %f  \n",abs_time/1000.0));
    }
    TRACE_LOG((stdout,"\n"));
    
    glite_io_finalize();
    
    return 0;
}
