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
#include <errno.h>
#include <string.h>
#include <unistd.h>



#include <glite/data/hydra/c/eds-simple.h>
#include <gfal_api.h>
#include <gfal_internals.h> /* without warranty */


#define PROGNAME     "glite-eds-rm"
#define PROGAUTHOR   "(C) EGEE"

#define GFAL_LFN_LENGTH		  256

#define TOOL_USER_VERBOSE   "__GLITE_EDS_VERBOSE"

#define true 1
#define false 0


#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

static void print_usage_and_die(FILE * out){
    fprintf (out, "\n");
    fprintf (out, "usage: %s <remotefilename> [-i <id>]\n", PROGNAME);
    fprintf (out, "  -i <id>        : the ID to use to look up the decryption key of this file "
        "(defaults to the remotefilename's GUID).\n");
    fprintf (out, " Optional parameters:\n");
    fprintf (out, "  -h      : print this screen\n");
    fprintf (out, "  -q      : quiet mode\n");
    fprintf (out, "  -v      : verbose mode\n");
    fprintf (out, "  -V      : print version and exit\n");

    exit((out == stdout) ? 0 : -1);
}

int main(int argc, char* argv[]) {

    char remotefilename[GFAL_LFN_LENGTH + 7];
    char errbuf[256];

    struct timeval abs_start_time;
    struct timeval abs_stop_time;
    struct timezone tz;

    char *id = NULL;
    int silent = false;

    int flag;
    while ((flag = getopt (argc, argv, "qhvVi:")) != -1) {
        switch (flag) {
            case 'q':
                silent = true;
                unsetenv(TOOL_USER_VERBOSE);
                break;
            case 'h':
                print_usage_and_die(stdout);
                break;
            case 'i':
                id = strdup(optarg);
                if (id == NULL) {
                    TRACE_ERR((stderr, "Failed duplicate -i argument, parameter %d chars\n",
                            strlen(optarg)));
                    exit(-1);
                }
                break;
            case 'v':
                silent = false;
                setenv(TOOL_USER_VERBOSE, "YES", 1);
                break;
            case 'V':
                fprintf (stdout, "<%s> Version %s by %s\n",
                        PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
                exit(0);
            default:
                print_usage_and_die(stderr);
            break;
        } // End Switch
    } // End while

    if (argc != (optind+1)) {
        print_usage_and_die(stderr);
    }

    // Copy file name
    // -------------------------------------------------------------------------
    if (canonical_url(argv[optind], "lfn", remotefilename, sizeof(remotefilename),
                errbuf, sizeof(errbuf)) < 0) {
            TRACE_ERR((stderr,"Error in Remote File Name %s. Error is %s (code: %d)\"\n",
                    remotefilename, errbuf, errno));
        goto err;
    }


    gettimeofday(&abs_start_time,&tz);

    // Fetch the guid of the file
    // -------------------------------------------------------------------------
    if(id == NULL) {
        if (strncmp(remotefilename, "lfn:", 4) == 0) {
            if ((id = guidfromlfn(remotefilename + 4, errbuf, sizeof(errbuf))) == NULL) {
                TRACE_ERR((stderr,"Cannot get guid for LFN-file %s. Error is %s (code: %d)\"\n",
                            remotefilename + 4, errbuf, errno));
                goto err;
            }
        } else if (strncmp(remotefilename, "guid:", 5) == 0) {
            if ((id = strdup(remotefilename + 5)) == NULL) {
                TRACE_ERR((stderr,"Failed duplicate guid, length %d\n",
                            strlen(remotefilename + 5)));
                goto err;
            }
        } else {
            TRACE_ERR((stderr,"Protocol not supported: %s. Use LFN- or GUID-format.\n",
                        remotefilename));
            goto err;
        }
    }

    // Unlink entries in Hydra
    // -------------------------------------------------------------------------
    int failures = 0;
    char *error;
    if (glite_eds_unregister(id, &error))
    {
        TRACE_ERR((stderr, "WARNING: during glite_eds_unregister: %s\n", error));
        free(error);
        failures++;
    }

    // Unlink remote file
    // -------------------------------------------------------------------------

    if (gfal_unlink(remotefilename)<0) {
        TRACE_ERR((stderr,"WARNING: cannot unlink remote file %s. Error is %s (code: %d)\"\n",
                    remotefilename, strerror(errno), errno));
        failures++;
    }

    if (failures > 1)
        goto err;

    gettimeofday (&abs_stop_time, &tz);
    float abs_time= ((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec) *1000 
        + (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));

    if (!silent) {
        TRACE_LOG((stdout,"\nUnlink Completed:\n\n"));
        TRACE_LOG((stdout,"  File          : %s  \n",remotefilename));
        if (abs_time != 0) {
            TRACE_LOG((stdout,"  Time [s]      : %f  \n",abs_time/1000.0));
        }
        TRACE_LOG((stdout,"\n"));
    }

    return 0;

    // Error handling
    // -------------------------------------------------------------------------

err:
    return -1;
}

/* vim: set et sw=4 ts=4: */
