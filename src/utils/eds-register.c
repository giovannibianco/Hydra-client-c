/*
 * Copyright (c) Members of the EGEE Collaboration. 2006-2010.
 * See http://www.eu-egee.org/partners/ for details on the copyright
 * holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#include <glite/data/catalog/c/catalog-simple.h>

#define PROGNAME     "glite-eds-key-register"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

#define TRANSFERBLOCKSIZE 10000000

static void print_usage_and_die(FILE * out){
    fprintf(out, "\n");
    fprintf(out, "usage: %s <ID>\n", PROGNAME);
    fprintf(out, "  ID      : The remote ID (lfn or GUID) of the key \n");
    fprintf(out, " Optional parameters:\n");
    fprintf(out, "  -c name : cipher name to use\n");
    fprintf(out, "  -k n    : key size to use in bits\n");
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
    int silent = 0;

    while ((flag = getopt (argc, argv, "qhvVc:k:")) != -1) {
        switch (flag) {
            case 'q':
                silent = 1;
                break;
            case 'h':
                print_usage_and_die(stdout);
                break;
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
            case 'V':
				fprintf(stdout, "<%s> Version %s by %s\n",
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

    // Do the registration
    // -------------------------------------------------------------------------
    char *error;
    int errclass;

    if (0 != (errclass = glite_eds_register(argv[optind], cipher, key_size, &error)))
    {
        TRACE_ERR((stderr, "Error during glite_eds_register: %s\n", error));
        free(error);
        if (errclass != GLITE_CATALOG_EXCEPTION_EXISTS) 
        {
            if (glite_eds_unregister(argv[optind], &error))
            {
                TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n", error));
                free(error);
            }
        }
        return -1;
    }

    if(!silent) {
        fprintf(stdout, "A key has been generated and registered for ID '%s'\n", argv[optind]);
    }
    
    return 0;
}
