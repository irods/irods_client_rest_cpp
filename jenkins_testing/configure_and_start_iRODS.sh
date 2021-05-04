#!/bin/bash -e
# run as root
service postgresql start
su - postgres -c 'psql -f /tmp/ICAT.sql'
python /var/lib/irods/scripts/setup_irods.py < /var/lib/irods/packaging/localhost_setup_postgres.input
pgrep irodsServer || su - irods -c '~/irodsctl start'
