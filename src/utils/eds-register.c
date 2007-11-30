/*
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://eu-egee.org/partners/ for details on the copyright holders.
 *  For license conditions see the license file or http://eu-egee.org/license.html
 *
 *  gLite Encrypted Data Storage key generation and registration client.
 *
 *  Authors: 
 *      Zoltan Farkas <Zoltan.Farkas@cern.ch>
 *      Akos Frohner <Akos.Frohner@cern.ch>
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

#include <glite/data/hydra/c/eds-simple.h>

#define PROGNAME     "glite-eds-key-register"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE 10000000

static void print_usage_and_die(FILE * out){
    fprintf(out, "\n");
    fprintf(out, "<%s> Version %s by %s\n", PROGNAME, PACKAGE_VERSION, PROGAUTHOR);
    fprintf(out, "usage: %s <ID>\n", PROGNAME);
    fprintf(out, "  ID      : The remote ID (lfn or GUID) of the key \n");
    fprintf(out, " Optional parameters:\n");
    fprintf(out, "  -v      : verbose mode\n");
    fprintf(out, "  -q      : quiet mode\n");
/***
    fprintf(out, "  -s URL  : the Hydra KeyStore to talk to\n");
***/
    fprintf(out, "  -c name : cipher name to use\n");
    fprintf(out, "  -k n    : key size to use in bits\n");
    fprintf(out, "  -h      : print this screen\n");
    if (out == stdout) {
        exit(0);
    }
    exit(-1);
}

int main(int argc, char **argv)
{
    int flag, key_size = 0;
    char *cipher = NULL;
    int silent = 0;
/***
    char *service_endpoint = NULL;
***/

/***
    while ((flag = getopt (argc, argv, "qhvc:k:s:")) != -1) {
***/
    while ((flag = getopt (argc, argv, "qhvc:k:")) != -1) {
        switch (flag) {
            case 'q':
                silent = 1;
                break;
            case 'h':
                print_usage_and_die(stdout);
                break;
/***
            case 's':
                service_endpoint = strdup(optarg);
                break;
***/
            case 'v':
                silent = 0;
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
    
    if (argc != (optind+1)) {
        print_usage_and_die(stderr);
    }

    // Do the registration
    // -------------------------------------------------------------------------
    char *error;

    if (glite_eds_register(argv[optind], cipher, key_size, &error))
    {
        TRACE_ERR((stderr, "Error during glite_eds_register: %s\n", error));
        free(error);
        if (glite_eds_unregister(argv[optind], &error))
        {
            TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n", error));
            free(error);
        }
        return -1;
    }

    if(!silent) {
        fprintf(stdout, "A key has been generated and registered for ID '%s'\n", argv[optind]);
    }
    
    return 0;
}
