#!/bin/bash
#
# Copyright (c) Members of the EGEE Collaboration. 2004-2007.
# See http://public.eu-egee.org/partners/ for details on 
# the copyright holders.
# For license conditions see the license file or
# http://eu-egee.org/license.html
#
# Authors: 
#      Akos Frohner <Akos.Frohner@cern.ch>
#

TEST_MODULE='glite-data-hydra-cli'
TEST_REQUIRES='glite-eds-key-register glite-eds-key-unregister glite-eds-encrypt glite-eds-decrypt uuidgen voms-proxy-info openssl'

if [ -z "$GLITE_LOCATION" ]; then
    GLITE_LOCATION=$(dirname $0)/../../stage
fi
source $GLITE_LOCATION/share/test/utils/shunit

# iteration number of the performance tests
ITERATION=${ITERATION:-10}

GUID=$(uuidgen)

export GLITE_SD_PLUGIN='file'
export GLITE_SD_SERVICES_XML=$PWD/services.xml
export TEST_CERT_DIR=${TEST_CERT_DIR:-$(dirname $0)/../../org.glite.data.hydra-service/build/certs}

export X509_CERT_DIR=$TEST_CERT_DIR/grid-security/certificates
export X509_VOMS_DIR=$TEST_CERT_DIR/grid-security/vomsdir

test_success 'Endpoint: http.*://localhost:8.*/1/glite-data-hydra-service' \
    glite-sd-query -t org.glite.Metadata

function test_17023 {
    echo "####################################"
    echo "# Simple en-de-cryption test, #17023"
    echo "####################################"
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem

    test_success 'registered'  glite-eds-key-register -v $GUID

    echo 'testdata' >$tempbase.input
    test_success 'encrypted' glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted

    test_success 'decrypted' glite-eds-decrypt -v $GUID $tempbase.encrypted $tempbase.output

    cmp $tempbase.input $tempbase.output
    if [[ $? == 0 ]]; then
        echo 'File en-de-cryption worked correctly.'
    else
        echo 'En-de-crypted file is not same as the original!' >&2
    fi

    test_success 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_encryption_speed {
    echo "############################"
    echo "# En-de-cryption speed test."
    echo "############################"
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    test_success 'registered'  glite-eds-key-register -v $GUID

    echo 'testdata' >$tempbase.input
    time (for i in $(seq -f '%04g' 1 $ITERATION); do \
        glite-eds-encrypt $GUID $tempbase.input $tempbase-$i.encrypted; \
        glite-eds-decrypt $GUID $tempbase-$i.encrypted $tempbase-$i.output; \
        cmp $tempbase.input $tempbase-$i.output; \
        if [[ $? != 0 ]]; then echo "En/decryption failure at $i!"; $FAILONERROR 2; fi; \
    done)

    test_success 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_registration_speed {
    echo "#####################################################################"
    echo "# Generating and registrating $ITERATION keys and then removing them."
    echo "#####################################################################"
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    time (for i in $(seq -f '%04g' 1 $ITERATION); do glite-eds-key-register $GUID$i; done)
    time (for i in $(seq -f '%04g' 1 $ITERATION); do glite-eds-key-unregister $GUID$i; done)
}

function test_17024 {
    echo "##################################################"
    echo "# Permission test with different identities #17024"
    echo "##################################################"

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'registered'  glite-eds-key-register -v $GUID

    test_success 'Base perms: user pdrwl-gs, group --------, other --------' \
        glite-eds-getacl -v $GUID
    echo 'testdata' >$tempbase.input
    test_success 'encrypted' glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted
    rm $tempbase.encrypted

    echo '+-----------------------------------'
    echo '| permit getKey via group permission'
    echo '+-----------------------------------'
    export X509_USER_PROXY=$TEST_CERT_DIR/home/user01-voms.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/user01-voms.pem"
    # this one should fail
    test_failure 'Error during glite_eds_encrypt_init' \
        glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'Using endpoint' \
        glite-eds-chmod -v g+g $GUID
    test_success 'Base perms: user pdrwl-gs, group ------g-, other --------' \
        glite-eds-getacl -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/user01-voms.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/user01-voms.pem"
    # this should work now
    test_success 'encrypted' glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted
    rm $tempbase.encrypted

    echo '+----------------------'
    echo '| permit getKey via ACL'
    echo '+----------------------'
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'Using endpoint' \
        glite-eds-chmod -v g-g $GUID
    test_success 'Base perms: user pdrwl-gs, group --------, other --------' \
        glite-eds-getacl -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem"
    # this one should fail
    test_failure 'Error during glite_eds_encrypt_init' \
        glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    user02dn=$(openssl x509 -in $TEST_CERT_DIR/home/user02cert.pem -noout -subject)
    test_success 'Changed ACLs' \
        glite-eds-setacl -v -m "${user02dn:9}:g" $GUID
    test_success "${user02dn:9}:------g-" \
        glite-eds-getacl -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem"
    # this should work now
    test_success 'encrypted' glite-eds-encrypt -v $GUID $tempbase.input $tempbase.encrypted
    rm $tempbase.encrypted

    echo '+------------'
    echo '| cleaning up'
    echo '+------------'
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'unregistered' glite-eds-key-unregister -v $GUID

    rm $tempbase.input
}

function test_17026 {
    echo "#####################################"
    echo "# Test for #17026"
    echo "#####################################"

    export X509_USER_PROXY=$TEST_CERT_DIR/home/user01-voms.pem
    test_success 'registered'  glite-eds-key-register -v $GUID

    test_success "Base perms" \
        glite-eds-getacl -v $GUID

    test_success 'Changed ACLs' \
        glite-eds-setacl -v -m a:r $GUID
    test_success 'Using endpoint' \
        glite-eds-setacl -v -d a:r $GUID

    test_success 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_17027 {
    echo "#####################################"
    echo "# Test for #17027"
    echo "#####################################"

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    test_success 'registered'  glite-eds-key-register -v $GUID

    # https://savannah.cern.ch/bugs/?31800
    test_failure "identity  : /C=UG/L=Tropic/O=Utopia/OU=Relaxation/CN=$LOGNAME" \
        voms-proxy-info -all 
    test_success "# User: /C=UG/L=Tropic/O=Utopia/OU=Relaxation/CN=$LOGNAME" \
        glite-eds-getacl -v $GUID

    # https://savannah.cern.ch/bugs/?31800
    test_failure 'attribute : /org.acme' \
        voms-proxy-info -debug -all
    test_success '# Group: /org.acme' \
        glite-eds-getacl -v $GUID

    test_success 'unregistered' glite-eds-key-unregister -v $GUID
}


function test_remove_nonexistant_entry {
    echo "###############################################"
    echo "# Test for error in removing not existing entry"
    echo "###############################################"
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    test_failure 'Error.*could not be found' glite-eds-key-unregister -v DOES_NOT_EXISTS
}

function test_setPermission_checkPermission {
    echo "###########################################"
    echo "# Test for setPermission() permission check"
    echo "###########################################"
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'registered'  glite-eds-key-register -v $GUID

    user02dn=$(openssl x509 -in $TEST_CERT_DIR/home/user02cert.pem -noout -subject)

    export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem"
    test_failure 'ERROR eds-setacl' \
        glite-eds-setacl -v -m "${user02dn:9}:g" $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'Changed ACLs' \
        glite-eds-setacl -v -m "${user02dn:9}:g" $GUID
    test_success "${user02dn:9}:------g-" \
        glite-eds-getacl -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_getPermission_checkPermission {
    echo "###########################################"
    echo "# Test for getPermission() permission check"
    echo "###########################################"
    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'registered'  glite-eds-key-register -v $GUID
    test_success " Base perms:" glite-eds-getacl -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/user02-voms.pem"
    test_failure 'ERROR eds-getacl' glite-eds-getacl -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'unregistered' glite-eds-key-unregister -v $GUID
}

function test_createEntry_checkPermission {
    echo "#########################################"
    echo "# Test for createEntry() permission check"
    echo "#########################################"

    # the test service is configured to accept create entry
    # requests from the /org.acme VO only

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-coyote.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-coyote.pem"
    test_failure 'Error'  glite-eds-key-register -v $GUID
    test_failure 'Error' glite-eds-key-unregister -v $GUID

    export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem
    echo "export X509_USER_PROXY=$TEST_CERT_DIR/home/voms-acme.pem"
    test_success 'registered'  glite-eds-key-register -v $GUID
    test_success 'unregistered' glite-eds-key-unregister -v $GUID
}

test_17023
test_encryption_speed
test_registration_speed
test_17024
test_17026
test_17027
test_remove_nonexistant_entry
test_setPermission_checkPermission
test_getPermission_checkPermission
test_createEntry_checkPermission

test_summary
