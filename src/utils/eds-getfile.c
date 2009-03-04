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
#include <gfal_internals.h> /* without warranty */

#define PROGNAME     "glite-eds-get"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE     10000000
#define GFAL_LFN_LENGTH		  256

#define TOOL_USER_VERBOSE   "__GLITE_EDS_VERBOSE"

#define true	1
#define false	0


static void print_usage_and_die(FILE * out) {
    fprintf (out, "\n");
    fprintf (out, "usage: %s <remotefilename> <localfilename> [-i <id>]\n", PROGNAME);
    fprintf(out, "  -i <id>        : the ID to use to look up the decryption key of this file "
            "(defaults to the remotefilename's GUID).\n");
    fprintf (out, " Optional parameters:\n");
    fprintf (out, "  -h      : print this screen\n");
    fprintf (out, "  -q      : quiet mode\n");
    fprintf (out, "  -v      : verbose mode\n");
    fprintf (out, "  -V      : print version and exit\n");

    exit((out == stdout) ? 0 : -1);
}

int main(int argc, char* argv[])
{
    char localfilename[GFAL_LFN_LENGTH];
    char remotefilename[GFAL_LFN_LENGTH + 7];
    char errbuf[256];

    struct timeval abs_start_time;
    struct timeval abs_stop_time;
    struct timezone tz;

    char *id = NULL;
    int silent     = false;

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
                setenv(TOOL_USER_VERBOSE, "YES", 1);
                silent = false;
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

    if (argc != (optind+2)) {
        print_usage_and_die(stderr);
    }

    // Copy Remote file name
    // -------------------------------------------------------------------------
    if (canonical_url(argv[optind], "lfn", remotefilename, sizeof(remotefilename),
                errbuf, sizeof(errbuf)) < 0) {
            TRACE_ERR((stderr,"Error in Remote File Name %s. Error is %s (code: %d)\"\n",
                    remotefilename, errbuf, errno));
        goto err;
    }

    // Copy local file name
    // -------------------------------------------------------------------------
    if (strlen(argv[optind + 1]) > GFAL_LFN_LENGTH - 1) {
        TRACE_ERR((stderr, "Local filename is too long (more than %d chars)!\n",
                    GFAL_LFN_LENGTH - 1));
        goto err;
    }
    strcpy(localfilename, argv[optind + 1]);

    char *buffer = (char *)malloc(TRANSFERBLOCKSIZE);
    if (!buffer) {
        TRACE_ERR((stderr, "Failed to allocate transfer buffer of size %d bytes!\n",
                    TRANSFERBLOCKSIZE));
        goto err;
    }

    gettimeofday(&abs_start_time,&tz);

    // Open remote file
    // -------------------------------------------------------------------------
    int fh = gfal_open(remotefilename,O_RDONLY, 0);
    if (fh < 0) {
        TRACE_ERR((stderr,"Cannot Open Remote File %s. Error is %s (code: %d)\"\n",
                    remotefilename, strerror(errno), errno));
        goto err;
    }

    // Fetch the guid of the file
    // -------------------------------------------------------------------------
    if (id == NULL) {
        if (strncmp(remotefilename, "lfn:", 4) == 0) {
            if ((id = guidfromlfn(remotefilename + 4, errbuf, sizeof(errbuf))) == NULL) {
                TRACE_ERR((stderr,"Cannot get guid for LFN-file %s. Error is %s (code: %d)\"\n",
                            remotefilename + 4, errbuf, errno));
                goto err_close_gfal;
            }
        } else if (strncmp(remotefilename, "guid:", 5) == 0) {
            if ((id = strdup(remotefilename + 5)) == NULL) {
                TRACE_ERR((stderr,"Failed duplicate guid, length %d\n",
                            strlen(remotefilename + 5)));
                goto err_close_gfal;
            }
        } else {
            TRACE_ERR((stderr,"Protocol not supported: %s. Use LFN- or GUID-format.\n",
                        remotefilename));
            goto err_close_gfal;
        }
    }

    // Initialize eds library
    // -------------------------------------------------------------------------
    char *error;
    EVP_CIPHER_CTX *dctx;

    dctx = glite_eds_decrypt_init(id, &error);
    if (dctx == NULL) {
        TRACE_ERR((stderr, "Error during glite_eds_decrypt_init: %s\n", error));
        goto err_close_gfal;
    }

    // Get remote file size
    // -------------------------------------------------------------------------
    struct stat statbuf;
    if (gfal_stat(remotefilename, &statbuf) < 0) {
        TRACE_ERR((stderr,"Cannot Get Remote File Stat. Error is \"%s (code: %d)\"\n",
                    strerror(errno), errno));
        goto err_free_eds;
    } 
    off_t size = statbuf.st_size;

    // Open local file
    // -------------------------------------------------------------------------
    int fdump = open(localfilename,O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fdump < 0) {
        TRACE_ERR((stderr,"Cannot Create Local File %s. Error is \"%s (code: %d)\"\n",
            localfilename, strerror(errno), errno));
        goto err_free_eds;
    }

    off_t bytesread = 0;
    off_t byteswritten = 0;

    // Read Remote File
    // -------------------------------------------------------------------------
    while (bytesread < size) {
        int nread = gfal_read(fh,buffer,TRANSFERBLOCKSIZE);
        if (nread <= 0) {
            if (nread == 0) errno = ENODATA;
            TRACE_ERR((stderr,"Fatal error during remote read. Error is \"%s (code: %d)\"\n",
                        strerror(errno), errno));
            TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes read!\n",
                        bytesread, size));
            goto err_close_fdump;
        }

        int dec_buffer_size;
        char *dec_buffer;
        if (glite_eds_decrypt_block(dctx, buffer, nread, &dec_buffer, &dec_buffer_size, &error)) {
            TRACE_ERR((stderr, "Error during glite_eds_encrypt_block: %s\n", error));
            goto err_close_fdump;
        }

        int nwrite = write(fdump, dec_buffer, dec_buffer_size);
        free(dec_buffer);
        if (nwrite != dec_buffer_size) {
            TRACE_ERR((stderr,"Fatal error during local write. Error is \"%s (code: %d)\"\n",
                        strerror(errno), errno));
            TRACE_ERR((stderr,"Transfer Finished after %lld bytes written!\n", byteswritten));
            goto err_close_fdump;
        }

        bytesread += nread;
        byteswritten += nwrite;

        // Print Progress Bar
        // ---------------------------------------------------------------------
        if (!silent) {
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
            float abs_time=((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec) * 1000 +
                        (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));
            TRACE_LOG((stdout,"| %.02f %% [%.01f Mb/s]\r",
                        100.0*bytesread/size,(float)bytesread/abs_time/1000.0));
            fflush(stdout);
        }  // End Progress Bar
    } // End While

    TRACE_LOG((stdout,"\n"));

    free(buffer);

    // Write final block
    // -------------------------------------------------------------------------
    int final_buf_size;
    char *final_buf;
    if (glite_eds_decrypt_final(dctx, &final_buf, &final_buf_size, &error)) {
        TRACE_ERR((stderr, "Error during glite_eds_encrypt_final: %s\n", error));
        goto err_close_fdump;
    }

    if (final_buf_size) {
        int nwrite = write(fdump, final_buf, final_buf_size);
        free(final_buf);
        if (nwrite != final_buf_size) {
            TRACE_ERR((stderr,"Fatal error during local write. Error is \"%s (code: %d)\"\n",
                        strerror(errno), errno));
            TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes read!\n",
                        bytesread, size));
            goto err_close_fdump;
        }
        byteswritten += nwrite;
    }

    // Close Local File
    // -------------------------------------------------------------------------
    if (close(fdump)) {
        TRACE_ERR((stderr,"WARNING: Error in Closing Local File. Error is \"%s (code: %d)\"\n",
            strerror(errno), errno));
    }

    // Shut down encryption
    // -------------------------------------------------------------------------
    glite_eds_finalize(dctx, &error);

    // Close Remote File
    // -------------------------------------------------------------------------
    if(gfal_close(fh)) {
        TRACE_ERR((stderr,"WARNING: Error in Closing Remote File. Error is \"%s (code: %d)\"\n",
                    strerror(errno), errno));
    }

    gettimeofday (&abs_stop_time, &tz);
    float abs_time=((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec) * 1000 +
                (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));

    if (!silent) {
        TRACE_LOG((stdout, "\nTransfer Completed:\n\n"));
        TRACE_LOG((stdout, "  LFN                     : %s  \n", remotefilename));
        TRACE_LOG((stdout, "  GUID                    : %s  \n", id));

        char **replicas;
        char **p;
        if((replicas = gfal_get_replicas(remotefilename, id, errbuf, sizeof(errbuf))) != NULL) {
            for(p = replicas; *p != NULL; p++) {
                TRACE_LOG((stdout, "  SURL                    : %s  \n", *p));
            }
        }
        TRACE_LOG((stdout, "  Remote Read [bytes]     : %lld\n", bytesread));
        TRACE_LOG((stdout, "  Locally Written [bytes] : %lld\n", byteswritten));
        if (abs_time != 0) {
            TRACE_LOG((stdout, "  Eff.Transfer Rate[Mb/s] : %f  \n",
               bytesread / abs_time / 1000.0));
        }
        TRACE_LOG((stdout,"\n"));
    }

    return 0;

    // Error handling
    // -------------------------------------------------------------------------

err_close_fdump:
    close(fdump);
err_free_eds:
    glite_eds_finalize(dctx, &error);
err_close_gfal:
    gfal_close(fh);
err:
    return -1;
}

/* vim: set et sw=4 ts=4: */
