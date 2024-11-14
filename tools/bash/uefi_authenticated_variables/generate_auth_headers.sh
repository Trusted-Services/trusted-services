#!/usr/bin/env bash
#
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Relationship of the generated variable write requests:
#
# @startuml
# left to right direction

# rectangle PK1
# rectangle PK2
# rectangle PK3
# rectangle KEK
# rectangle db1
# rectangle db2
# rectangle var

# PK1  --> PK1 : delete
# PK1  --> PK2
# PK1  --> KEK
# PK1  --> db2
# PK2  --> PK2 : delete
# PK1 --> KEK : delete
# KEK --> db1
# var --> var : append
# var --> var : append_old
# var --> var : delete
# var --> var : delete_old
# @enduml

# Check environment
which sign-efi-sig-list     || { echo "Please install 'efitools' Minimum version: 1.8.1" && exit 1; }
which sbsign                || { echo "Please install 'efitools' Minimum version: 1.8.1" && exit 1; }
which openssl               || { echo "Please install 'openssl'" && exit 1; }


HEADER_FOLDER=auth_vectors
TEMP_FOLDER=temp_files

# Generate a certificate with a public key and it's private key file
generate_key_cert() {
    name=$1

    openssl req -x509 -newkey rsa:2048 -subj "/CN=Test $name/" -keyout $name.key -out $name.crt -days 3650 -nodes -sha256

    # Create a concatenated {CRT,KEY} PEM file and also a DER from the certificate for later use
    cat $name.crt $name.key > $name.pem
    openssl x509 -in $name.crt -out $name.der -outform DER
}

mkdir -p ${HEADER_FOLDER}
mkdir -p ${TEMP_FOLDER}
pushd ${TEMP_FOLDER}

# Create signer certificates
generate_key_cert "PK1"
generate_key_cert "PK2"
generate_key_cert "PK3"
generate_key_cert "KEK"
generate_key_cert "DB1"
generate_key_cert "DB2"
generate_key_cert "VAR"

# Create data file for the custom variable
cat <<EOF > var_data_part01.txt
The term 'trusted service' is used as a general name for a class of application that runs in an isolated
processing environment. Other applications rely on trusted services to perform security related operations in
a way that avoids exposing secret data beyond the isolation boundary of the environment. The word 'trusted'
does not imply anything inherently trustworthy about a service application but rather that other applications
put trust in the service. Meeting those trust obligations relies on a range of hardware and firmware
implemented security measures.
EOF

cat <<EOF > var_data_part02.txt
The Arm Application-profile (A-profile) architecture, in combination with standard firmware, provides a range
of isolated processing environments that offer hardware-backed protection against various classes of attack.
Because of their strong security properties, these environments are suitable for running applications that have
access to valuable assets such as keys or sensitive user data. The goal of the Trusted Services project is
to provide a framework in which security related services may be developed, tested and easily deployed to
run in any of the supported environments. A core set of trusted services are implemented to provide basic
device security functions such as cryptography and secure storage.

Example isolated processing environments are:

    - **Secure partitions** - secure world isolated environments managed by a secure partition manager
    - **Trusted applications** - application environments managed by a TEE
    - **VM backed container** - container runtime that uses a hypervisor to provide hardware backed container isolation

The default reference system, used for test and development, uses the Secure Partition Manager configuration
of OP-TEE to manage a set of secure partitions running at S-EL0. The secure partitions host service providers
that implement PSA root-of-trust services. Services may be accessed using client-side C bindings that expose PSA
Functional APIs. UEFI SMM services are provided by the SMM Gateway.
EOF

cat var_data_part01.txt var_data_part02.txt > var_data.txt

# Generate EFI signature list from the certificates for each keystore variable and an empty esl for delete requests
cert-to-efi-sig-list PK1.crt PK1.esl
cert-to-efi-sig-list PK2.crt PK2.esl
cert-to-efi-sig-list PK3.crt PK3.esl
cert-to-efi-sig-list KEK.crt KEK.esl
cert-to-efi-sig-list DB1.crt DB1.esl
cert-to-efi-sig-list DB2.crt DB2.esl
touch NULL.esl

# Add another signature list before the correct KEK list to test if multiple lists are supported
cat PK3.esl KEK.esl > KEK_concatenated.esl

sign-efi-sig-list -c PK1.crt -k PK1.key PK  PK1.esl               PK1.auth        ; sleep 1
sign-efi-sig-list -c PK1.crt -k PK1.key PK  NULL.esl              PK1_delete.auth ; sleep 1
sign-efi-sig-list -c PK1.crt -k PK1.key PK  PK2.esl               PK2.auth        ; sleep 1
sign-efi-sig-list -c PK2.crt -k PK2.key PK  NULL.esl              PK2_delete.auth ; sleep 1
sign-efi-sig-list -c PK3.crt -k PK3.key PK  PK3.esl               PK3.auth        ; sleep 1
sign-efi-sig-list -c PK1.crt -k PK1.key KEK KEK_concatenated.esl  KEK.auth        ; sleep 1
sign-efi-sig-list -c PK1.crt -k PK1.key KEK NULL.esl              KEK_delete.auth ; sleep 1
sign-efi-sig-list -c PK1.crt -k PK1.key db  DB2.esl               DB2.auth        ; sleep 1
sign-efi-sig-list -c KEK.crt -k KEK.key db  DB1.esl               DB1.auth        ; sleep 1

# GUID: Must be syncronized with m_common_guid in the tests
sign-efi-sig-list -c VAR.crt -k VAR.key -g '01234567-89AB-CDEF-0123-456789ABCDEF' -t 0 -a var var_data_part02.txt var_append_old.auth
sign-efi-sig-list -c VAR.crt -k VAR.key -g '01234567-89AB-CDEF-0123-456789ABCDEF' -t 0 var /dev/null var_delete_old.auth          ; sleep 1
sign-efi-sig-list -c VAR.crt -k VAR.key -g '01234567-89AB-CDEF-0123-456789ABCDEF' var var_data_part01.txt var.auth                ; sleep 1
sign-efi-sig-list -c VAR.crt -k VAR.key -g '01234567-89AB-CDEF-0123-456789ABCDEF' -a var var_data_part02.txt var_append.auth      ; sleep 1
sign-efi-sig-list -c VAR.crt -k VAR.key -g '01234567-89AB-CDEF-0123-456789ABCDEF' var /dev/null var_delete.auth                   ; sleep 1

# Generate C headers from the authentication headers for the tests
xxd -i PK1.auth            > ../${HEADER_FOLDER}/PK1.h
xxd -i PK2.auth            > ../${HEADER_FOLDER}/PK2.h
xxd -i PK3.auth            > ../${HEADER_FOLDER}/PK3.h
xxd -i PK1_delete.auth     > ../${HEADER_FOLDER}/PK1_delete.h
xxd -i PK2_delete.auth     > ../${HEADER_FOLDER}/PK2_delete.h
xxd -i KEK.auth            > ../${HEADER_FOLDER}/KEK.h
xxd -i KEK_delete.auth     > ../${HEADER_FOLDER}/KEK_delete.h
xxd -i DB2.auth            > ../${HEADER_FOLDER}/db2.h
xxd -i DB1.auth            > ../${HEADER_FOLDER}/db1.h
xxd -i var_append_old.auth > ../${HEADER_FOLDER}/var_append_old.h
xxd -i var_delete_old.auth > ../${HEADER_FOLDER}/var_delete_old.h
xxd -i var.auth            > ../${HEADER_FOLDER}/var.h
xxd -i var_append.auth     > ../${HEADER_FOLDER}/var_append.h
xxd -i var_delete.auth     > ../${HEADER_FOLDER}/var_delete.h
xxd -i var_data.txt        > ../${HEADER_FOLDER}/var_data.h

popd

# Add copyright to the beginning of the headers
current_year=$(date +"%Y")
copyright_header=$(cat <<-END
/*
 * Copyright (c) ${current_year}, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This file was generated by generate_auth_headers.sh
 */
END
)

for file in ./${HEADER_FOLDER}/*
do
    if test -f "$file"
    then
        echo -e "${copyright_header}\n\n$(cat $file)" > $file
    fi
done
