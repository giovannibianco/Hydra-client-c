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
#include <gfal_internals.h> /* without warranty */


#define PROGNAME     "glite-eds-put"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE 1000000
#define GFAL_LFN_LENGTH		  256

#define TOOL_USER_VERBOSE   "__GLITE_EDS_VERBOSE"

#define true	1
#define false	0


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

    exit((out == stdout) ? 0 : -1);
}

int main(int argc, char **argv)
{
    int flag, key_size = 0;
    char *cipher = NULL;
    int silent = false, reg_only = false;
    char localfilename[GFAL_LFN_LENGTH];
    char remotefilename[GFAL_LFN_LENGTH + 7];
    char errbuf[256];

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
                fprintf(stdout, "<%s> Version %s by %s\n",
                        PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
                exit(0);
            case 'c':
                cipher = strdup(optarg);
                if (id == NULL) {
                    TRACE_ERR((stderr, "Failed duplicate -c argument, parameter %d chars\n",
                            strlen(optarg)));
                    exit(-1);
                }
                break;
            case 'k':
                if (sscanf(optarg, "%d", &key_size) != 1) {
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
        TRACE_ERR((stderr, "Local filename is too long (more than %d chars)!\n",
                    GFAL_LFN_LENGTH - 1));
        goto err;
    }
    strcpy(localfilename, argv[optind]);

    // Copy Remote file name
    // -------------------------------------------------------------------------
    if (canonical_url(argv[optind + 1], "lfn", remotefilename, sizeof(remotefilename),
                errbuf, sizeof(errbuf)) < 0) {
            TRACE_ERR((stderr,"Error in Remote File Name %s. Error is %s (code: %d)\"\n",
                    remotefilename, errbuf, errno));
        goto err;
    }

    // Allocate transfer buffer
    // -------------------------------------------------------------------------
    char *buffer = (char *)malloc(TRANSFERBLOCKSIZE);
    if (!buffer) {
        TRACE_ERR((stderr, "Failed to allocate transfer buffer of size %d bytes!\n",
                    TRANSFERBLOCKSIZE));
        goto err;
    }

    // Open local file
    // -------------------------------------------------------------------------
    int fdump = open(localfilename, O_RDONLY);
    if (fdump < 0) {
        TRACE_ERR((stderr, "Cannot Open Local File %s. Error is \"%s (code: %d)\"\n",
                    localfilename, strerror(errno), errno));
        goto err;
    }

    // Get local file size
    // -------------------------------------------------------------------------
    struct stat st_buf;
    if (fstat(fdump, &st_buf) < 0) {
        TRACE_ERR((stderr, "Fatal error during fstat on local file %s. Error is \"%s (code: %d)\"\n",
                    localfilename, strerror(errno), errno));
        goto err_close_fdump;
    }

    off_t size = st_buf.st_size;

    gettimeofday(&abs_start_time,&tz);

    // Open remote file
    // -------------------------------------------------------------------------
    int fh = gfal_open (remotefilename, O_WRONLY|O_CREAT, 0644);
    if (fh < 0 ) {
        TRACE_ERR((stderr,"Cannot Create Remote File %s. Error is \"%s (code: %d)\"\n",
                    remotefilename, strerror(errno), errno));
        goto err_close_fdump;
    }

    // Fetch the guid of the file
    // -------------------------------------------------------------------------$
    if (id == NULL) {
        if (strncmp(remotefilename, "lfn:", 4) == 0) {
            if ((id = guidfromlfn(remotefilename + 4, errbuf, sizeof(errbuf))) == NULL) {
                TRACE_ERR((stderr,"Cannot get guid for LFN-file %s. Error is %s (code: %d)\"\n",
                            remotefilename + 4, errbuf, errno));
                goto err_close_gfal;
            }
        } else {
            TRACE_ERR((stderr,"Protocol not supported: %s. Use LFN format.\n",
                        remotefilename));
            goto err_close_gfal;
        }
    }

    // Initialize eds library and register id
    // -------------------------------------------------------------------------
    char *error;
    EVP_CIPHER_CTX *ectx;

    ectx = glite_eds_register_encrypt_init(id, cipher, key_size, &error);
    if (ectx == NULL) {
        TRACE_ERR((stderr, "Error during glite_eds_register_encrypt_init: %s\n",
                    error));
        goto err_close_gfal;
    }

    off_t bytesread = 0;
    off_t byteswritten = 0;

    // Read Local File
    // -------------------------------------------------------------------------
    while (bytesread < size) {
        int nread = read(fdump,buffer,TRANSFERBLOCKSIZE);
        if (nread <= 0) {
            if (nread == 0) errno = ENODATA;
            TRACE_ERR((stderr,"Fatal error during local read. Error is \"%s (code: %d)\"\n",
                        strerror(errno), errno));
            TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes!\n",
                        bytesread, size));
            goto err_free_eds;
        }

        int enc_buffer_size;
        char *enc_buffer;

        if (reg_only) {  // don't actually do the encryption
            enc_buffer_size = nread;
            enc_buffer = buffer;
        } else {  // do the encryption
            if (glite_eds_encrypt_block(ectx, buffer, nread, &enc_buffer, 
                        &enc_buffer_size, &error)) {
                TRACE_ERR((stderr, "Error during glite_eds_encrypt_block: %s\n",
                            error));
                goto err_free_eds;
            }
        }

        int nwrite = gfal_write(fh, enc_buffer, enc_buffer_size);
        if (enc_buffer != buffer) free(enc_buffer);
        if (nwrite != enc_buffer_size) {
            TRACE_ERR((stderr, "Fatal error during remote write. Error is \"%s (code: %d)\"\n",
                        strerror(errno), errno));
            TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes!\n",
                        bytesread, size));
            goto err_free_eds;
        }
        bytesread += nread;
        byteswritten += nwrite;

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
    int final_buf_size = 0;
    char *final_buf;

    if (!reg_only) {
        if (glite_eds_encrypt_final(ectx, &final_buf, &final_buf_size, &error)) {
            TRACE_ERR((stderr, "Error during glite_eds_encrypt_final: %s\n",
                        error));
            goto err_free_eds;
        }
    }

    if (final_buf_size) {
        int nwrite = gfal_write(fh, final_buf, final_buf_size);
        free(final_buf);
        if (nwrite != final_buf_size) {
            TRACE_ERR((stderr, "Fatal error during remote write. Error is \"%s (code: %d)\"\n",
                        strerror(errno), errno));
            TRACE_ERR((stderr,"Transfer Finished after %lld/%lld bytes!\n",
                        bytesread, size));
            goto err_free_eds;
        }
        byteswritten += nwrite;
    }

    // Shut down encryption
    // -------------------------------------------------------------------------
    glite_eds_finalize(ectx, &error);

    // Close Remote File
    // -------------------------------------------------------------------------
    if (gfal_close(fh)) {
        TRACE_ERR((stderr,"WARNING: Error in Closing Remote File. Error is \"%s (code: %d)\"\n",
            strerror(errno), errno));
    }
    fh = -1;

    // Get File Status and check the file size
    // -------------------------------------------------------------------------
    struct stat statbuf;
    if (gfal_stat(remotefilename, &statbuf)) {
        TRACE_ERR((stderr, "Cannot Get Remote File Stat. Error is \"%s (code: %d)\"\n",
                    strerror(errno), errno));
        goto err_unregister_eds;
    }
    if (statbuf.st_size != byteswritten) {
        TRACE_ERR((stderr, "WARNING: Error in File Size: %lld written, %lld got by stat\n",
                    byteswritten, statbuf.st_size));
    }

    // Close Local File
    // -------------------------------------------------------------------------
    if (close(fdump)) {
        TRACE_ERR((stderr,"WARNING: Error in Closing Local File. Error is \"%s (code: %d)\"\n",
            strerror(errno), errno));
    }

    gettimeofday(&abs_stop_time, &tz);
    float abs_time = ((float)((abs_stop_time.tv_sec - abs_start_time.tv_sec)*1000
                + (abs_stop_time.tv_usec - abs_start_time.tv_usec) / 1000));

    if (!silent) {
        TRACE_LOG((stdout, "\nTransfer Completed:\n\n"));
        TRACE_LOG((stdout, "  LFN                     : %s  \n", remotefilename));
        TRACE_LOG((stdout, "  GUID                    : %s  \n", id));

        char **replicas;
        char **p;
        if ((replicas = gfal_get_replicas(remotefilename, id, errbuf, sizeof(errbuf))) != NULL) {
            for(p = replicas; *p != NULL; p++) {
                TRACE_LOG((stdout, "  SURL                    : %s  \n", *p));
            }
        }
        TRACE_LOG((stdout, "  Locally Read [bytes]    : %lld\n", bytesread));
        TRACE_LOG((stdout, "  Remote Written [bytes]  : %lld\n", byteswritten));
        if (abs_time != 0) {
            TRACE_LOG((stdout, "  Eff.Transfer Rate[Mb/s] : %f  \n",
                        byteswritten / abs_time / 1000.0));
        }
        TRACE_LOG((stdout,"\n"));
    }

    return 0;

    // Error handling
    // -------------------------------------------------------------------------
    // - try to clean all written or registered entries

err_free_eds:
    glite_eds_finalize(ectx, &error);
err_unregister_eds:
    if (glite_eds_unregister(id, &error)) {
        TRACE_ERR((stderr, "WARNING: Error during glite_eds_unregister: %s\n", error));
    }
err_close_gfal:
    if (fh >= 0) gfal_close(fh);
    if (gfal_unlink(remotefilename) < 0) {
        TRACE_ERR((stderr,"WARNING: cannot unlink remote file %s. Error is %s (code: %d)\"\n",
                    remotefilename, strerror(errno), errno));
    }
err_close_fdump:
    close(fdump);
err:
    return -1;
}

/* vim: set et sw=4 ts=4: */
