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

if [ $(cd .. && basename $PWD) = 'org.glite.data.hydra-cli' -a -d '../build' ]; then
    export PATH=$(dirname $0)/../build/src/utils/.libs:$PATH
    export LD_LIBRARY_PATH=$(dirname $0)/../build/src/c/.libs:$LD_LIBRARY_PATH
else
    export PATH=$(dirname $0)/../../stage/bin:$PATH
    export LD_LIBRARY_PATH=$(dirname $0)/../../stage/lib:$LD_LIBRARY_PATH
fi

tempbase=$(basename $0)-$$
trap "rm -rf $tempbase.*" EXIT
touch $tempstdout

# change it to 'exit' to fail the test
FAILONERROR=${FAILONERROR:-return}

# iteration number of the performance tests
ITERATION=${ITERATION:-100}

# test counts
TEST_ALL=0
TEST_BAD=0
TEST_GOOD=0

# should it print intermediate results
TEST_VERBOSE=${TEST_VERBOSE:-'no'}

function do_test {
    result="$1"
    shift
    TEST_ALL=$(($TEST_ALL + 1))
    echo ""
    echo "Command: $@"
    echo "Expected result: $result"
    $@ >$tempbase.stdout 2>&1 
    ret=$?
    [ 'yes' = "$TEST_VERBOSE" ] && sed -e 's/^/Output: /' $tempbase.stdout
    echo "$result" | egrep -qi 'error'
    if [ $? -eq 0 ]; then
        # it is expected to fail
        if [ $ret -eq 0 ]; then
            TEST_BAD=$(($TEST_BAD + 1))
            echo "NOT OK"
            return 1
        fi
    else
        # expected to succeed
        if [ $ret -ne 0 ]; then
            TEST_BAD=$(($TEST_BAD + 1))
            echo "NOT OK"
            return 1
        fi
    fi
    egrep -q "$result" $tempbase.stdout
    if [ $? -ne 0 ]; then
        TEST_BAD=$(($TEST_BAD + 1))
        echo "NOT OK"
        $FAILONERROR 2
    fi 
    TEST_GOOD=$(($TEST_GOOD + 1))
    echo "OK"
    return 0
}

function print_summary {
    echo ""
    echo "Tests run: $TEST_ALL, Success: $TEST_GOOD, Errors: $TEST_BAD"
    echo $(($TEST_GOOD * 100 / $TEST_ALL))"% success rate"
}

# check for required binaries
for prog in glite-eds-key-register glite-eds-key-unregister glite-eds-encrypt glite-eds-decrypt uuidgen egrep voms-proxy-info
do
    if [ ! -x "$(which $prog)" ]; then
        echo "Error: '$prog' not found'" >&2
        exit
    fi
done

TEST_CERT_DIR=$GLITE_LOCATION/share/test/certificates
export X509_CERT_DIR=$TEST_CERT_DIR/grid-security/certificates
export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
export VOMSDIR=$TEST_CERT_DIR/grid-security/vomsdir

echo "using cert and key:"
echo "    export X509_USER_PROXY=$X509_USER_PROXY"
echo "    export X509_CERT_DIR=$X509_CERT_DIR"
echo "    export VOMSDIR=$VOMSDIR"
echo ""

export GLITE_SD_VO='org.example.single'
export GLITE_SD_PLUGIN='file'
export GLITE_SD_SERVICES_XML=$(dirname $0)/services.xml

echo "Service-discovery settings:"
echo "    export GLITE_SD_VO='org.example.single'"
echo "    export GLITE_SD_PLUGIN='file'"
echo "    export GLITE_SD_SERVICES_XML=$(dirname $0)/services.xml"


GUID=$(uuidgen)

echo "The following service will be used for the test:"
do_test 'Endpoint: http.*://localhost:8.*/glite-data-hydra-service' \
    glite-sd-query -t org.glite.Metadata

function test_encryption {
    echo "#############################"
    echo "# Simple en-de-cryption test."
    echo "#############################"
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

function test_acl {
    echo "#####################################"
    echo "# ACL test with different identities."
    echo "#####################################"

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

#test_encryption
#test_registration_speed
#test_encryption_speed
#test_acl
test_17027
test_17026

print_summary
exit $TEST_BAD
