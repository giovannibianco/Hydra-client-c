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

function do_test {
    result="$1"
    shift
    echo ""
    echo "Command: $@"
    echo "Expected result: $result"
    $@ | tee $tempbase.stdout | sed -e 's/^/Output: /'
    #$@ >$tempbase.stdout
    if [ $? -ne 0 ]; then
        echo "NOT OK"
        return 1
    fi
    egrep -q "$result" $tempbase.stdout
    if [ $? -ne 0 ]; then
        echo "NOT OK"
        $FAILONERROR 2
    fi
    echo "OK"
    return 0
}

# check for required binaries
for prog in glite-eds-key-register glite-eds-key-unregister glite-eds-encrypt glite-eds-decrypt uuidgen egrep
do
    if [ ! -x "$(which $prog)" ]; then
        echo "Error: '$prog' not found'" >&2
        exit
    fi
done

TEST_CERT_DIR=$GLITE_LOCATION/share/test/certificates
export X509_CERT_DIR=$TEST_CERT_DIR/grid-security/certificates
export X509_USER_KEY=$TEST_CERT_DIR/home/userkey.pem
export X509_USER_CERT=$TEST_CERT_DIR/home/usercert.pem
export VOMSDIR=$TEST_CERT_DIR/grid-security/vomsdir


echo "using cert and key:"
echo "    export X509_USER_CERT=$X509_USER_CERT"
echo "    export X509_USER_KEY=$X509_USER_KEY"
echo "    export X509_CERT_DIR=$X509_CERT_DIR"
echo "    export VOMSDIR=$VOMSDIR"
echo ""
echo "voms-proxy-init using 'changeit' as password"
echo 'changeit' | voms-proxy-init -pwstdin 

export GLITE_SD_VO='org.example.single'
export GLITE_SD_PLUGIN='file'
export GLITE_SD_SERVICES_XML=$(dirname $0)/services.xml

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
    do_test 'unregistered' glite-eds-key-unregister -v $GUID
}

test_encryption
test_registration_speed
test_encryption_speed

