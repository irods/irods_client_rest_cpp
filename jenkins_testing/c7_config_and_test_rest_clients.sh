#!/bin/bash -e

## # ---------- start ICAT db and initialize for iRODS

cat >/tmp/ICAT.sql <<EOF
CREATE USER irods WITH PASSWORD 'testpassword';
CREATE DATABASE "ICAT";
GRANT ALL PRIVILEGES ON DATABASE "ICAT" TO irods;
EOF
su - postgres -c '
       pg_ctl initdb
       pg_ctl -D /var/lib/pgsql/data -l logfile start
       while ! psql -c "\l" >/dev/null 2>&1; do echo "waiting for database"; sleep 1; done
       psql -f /tmp/ICAT.sql'
 
echo "DATABASE INITed"

## # ---------- Copy NGINX template files into Centos7 / iRODS configuration
## 
sed -i.orig 's/\(listen\s*[^0-9]*\)80/\190/' /etc/nginx/nginx.conf
cp -rp /etc/irods/irods_client_rest_cpp.json{.template,}
cp -rp /etc/irods/irods_client_rest_cpp_reverse_proxy.conf.template /etc/nginx/conf.d/irods_client_rest_cpp_reverse_proxy.conf

## # ---------- Configure and start iRODS


python </var/lib/irods/packaging/localhost_setup_postgres.input  /var/lib/irods/scripts/setup_irods.py

su - irods -c '~/irodsctl restart'

## # ---------- Start REST
## 

/etc/init.d/irods_client_rest_cpp  start

## 
## # ---------- Start NGINX for reverse proxy (:80 -> :8080-8089)
## 

nginx  

# curl -X POST -H "Authorization: Basic $(echo -n rods:rods|base64)" http://localhost:80/irods-rest/0.9.0/auth -o /tmp/output 
# sleep 300d

su - irods -c "cd ~/scripts ; python run_tests.py --run_specific_test test_irods_client_rest_cpp"
