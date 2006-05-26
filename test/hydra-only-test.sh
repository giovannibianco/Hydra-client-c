#!/bin/bash
#
# Copyright (c) Members of the EGEE Collaboration. 2004.
# See http://public.eu-egee.org/partners/ for details on 
# the copyright holders.
# For license conditions see the license file or
# http://eu-egee.org/license.html
#
# Authors: 
#      Akos Frohner <Akos.Frohner@cern.ch>
#

TEST_MODULE='org.glite.data.hydra-cli'
TEST_REQUIRES='glite-eds-key-register glite-eds-key-unregister glite-eds-encrypt glite-eds-decrypt uuidgen voms-proxy-info'

if [ -z "$GLITE_LOCATION" ]; then
    GLITE_LOCATION=$(dirname $0)/../../stage
fi
source $GLITE_LOCATION/share/test/glite-data-util-c/shunit

# iteration number of the performance tests
ITERATION=${ITERATION:-100}

GUID=$(uuidgen)

do_test 'Endpoint: http.*://localhost:8.*/glite-data-hydra-service' \
    glite-sd-query -t org.glite.Metadata

function test_17023 {
    echo "####################################"
    echo "# Simple en-de-cryption test, #17023"
    echo "####################################"
    do_test 'registered'  glite-eds-key-register -v $GUID

    echo 'testdata' >$tempbase.input
    do_test 'encrypted' glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted

    do_test 'decrypted' glite-eds-decrypt -v $GUID $tempbase.encrypted $tempbase.output

    cmp $tempbase.input $tempbase.output
    if [[ $? == 0 ]]; then
        echo 'File en-de-cryption worked correctly.'
    else
        echo 'En-de-crypted file is not same as the original!' >&2
    fi

    do_test 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_encryption_speed {
    echo "############################"
    echo "# En-de-cryption speed test."
    echo "############################"
    do_test 'registered'  glite-eds-key-register -v $GUID

    echo 'testdata' >$tempbase.input
    time (for i in $(seq -f '%04g' 1 $ITERATION); do \
        glite-eds-encrypt $GUID $tempbase.input $tempbase-$i.encrypted; \
        glite-eds-decrypt $GUID $tempbase-$i.encrypted $tempbase-$i.output; \
        cmp $tempbase.input $tempbase-$i.output; \
        if [[ $? != 0 ]]; then echo "En/decryption failure at $i!"; $FAILONERROR 2; fi; \
    done)

    do_test 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_registration_speed {
    echo "#####################################################################"
    echo "# Generating and registrating $ITERATION keys and then removing them."
    echo "#####################################################################"
    time (for i in $(seq -f '%04g' 1 $ITERATION); do glite-eds-key-register $GUID$i; done)
    time (for i in $(seq -f '%04g' 1 $ITERATION); do glite-eds-key-unregister $GUID$i; done)
}

function test_17024 {
    echo "##################################################"
    echo "# Permission test with different identities #17024"
    echo "##################################################"

    do_test 'registered'  glite-eds-key-register -v $GUID

    do_test 'Base perms: user pdrw--gs, group --------, other --------' \
        glite-eds-getacl -v $GUID
    echo 'testdata' >$tempbase.input
    do_test 'encrypted' glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted
    rm $tempbase.encrypted

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms01-acme.pem
    # this one should fail
    do_test 'Error during glite_eds_encrypt_init' \
        glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem

    do_test 'Using endpoint' \
        glite-eds-chmod -v g+r $GUID
    do_test 'Base perms: user pdrw--gs, group --r-----, other --------' \
        glite-eds-getacl -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms01-acme.pem
    # this should work now
    do_test 'encrypted' glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem

    do_test 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_17027 {
    echo "#####################################"
    echo "# Test for #17027"
    echo "#####################################"

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms01-acme.pem
    do_test 'registered'  glite-eds-key-register -v $GUID

    do_test "identity  : /C=UG/L=Tropic/O=Utopia/OU=Relaxation/CN=$LOGNAME client01" \
        voms-proxy-info -all
    do_test "# User: /C=UG/L=Tropic/O=Utopia/OU=Relaxation/CN=$LOGNAME client01" \
        glite-eds-getacl -v $GUID

    do_test 'attribute : /org.acme' \
        voms-proxy-info -all
    do_test '# Group: /org.acme' \
        glite-eds-getacl -v $GUID

    do_test 'unregistered' glite-eds-key-unregister -v $GUID
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
}

function test_17026 {
    echo "#####################################"
    echo "# Test for #17026"
    echo "#####################################"

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms01-acme.pem
    do_test 'registered'  glite-eds-key-register -v $GUID

    do_test "Base perms" \
        glite-eds-getacl -v $GUID

    do_test 'Changed ACLs' \
        glite-eds-setacl -v -m a:r $GUID
    do_test 'Using endpoint' \
        glite-eds-setacl -v -d a:r $GUID

    do_test 'unregistered' glite-eds-key-unregister -v $GUID
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
}

test_17023
#test_registration_speed
#test_encryption_speed
test_17024
test_17027
test_17026

test_summary
