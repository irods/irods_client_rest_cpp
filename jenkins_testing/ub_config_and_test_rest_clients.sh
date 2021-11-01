#!/bin/bash -e

cat >/tmp/ICAT.sql <<EOF
CREATE USER irods WITH PASSWORD 'testpassword';
CREATE DATABASE "ICAT";
GRANT ALL PRIVILEGES ON DATABASE "ICAT" TO irods;
EOF

dir=$(dirname "$0")
"$dir"/configure_and_start_iRODS.sh
"$dir"/configure_and_start_NGINX.sh

# curl -X POST -H "Authorization: Basic $(echo -n rods:rods|base64)" http://localhost:80/irods-rest/0.9.0/auth -o /tmp/output 
# sleep 300d

su - irods -c "cd ~/scripts ; python run_tests.py --run_specific_test test_irods_client_rest_cpp"
