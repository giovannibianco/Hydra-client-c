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
 *  GLite Encrypted Data Storage rm client
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

#define PROGNAME     "glite-eds-key-unregister"
#define PROGAUTHOR   "(C) EGEE"

#define TRACE_LOG(a)  if(!silent) fprintf a
#define TRACE_ERR(a)  fprintf a

static void print_usage_and_die(FILE * out){
    fprintf (out, "\n");
    fprintf (out, "usage: %s <ID>\n", PROGNAME);
    fprintf(out, "  ID      : The remote ID (lfn or GUID) of the key \n");
    fprintf (out, " Optional parameters:\n");
    fprintf (out, "  -h      : print this screen\n");
    fprintf (out, "  -q      : quiet mode\n");
    fprintf (out, "  -v      : verbose mode\n");
    fprintf (out, "  -V      : print version and exit\n");
    if(out == stdout){
        exit(0);
    }
    exit(-1);
}

int main(int argc, char* argv[]) {

    int silent     = 0;
    char *error;
    
    int flag;
    while ((flag = getopt (argc, argv, "qhvV")) != -1) {
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

    if (glite_eds_unregister(argv[optind], &error))
    {
        TRACE_ERR((stderr, "Error during glite_eds_unregister: %s\n", error));
        free(error);
        return -1;
    }

    
    if(!silent) {
        fprintf(stdout, "The key has been unregistered for ID '%s'\n", argv[optind]);
    }
    
    return 0;
}
