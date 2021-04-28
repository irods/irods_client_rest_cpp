#!/bin/bash -e
TEMPLATE_LOC=/etc/irods
copy_to_dst() { local bn=$(basename "$1");
                local bn_strip=${bn%.template}
                cp "$1" "$2/$bn_strip" || return 1;
                echo "$2/$bn_strip"
}
RevProxyConf=${TEMPLATE_LOC}/irods-client-rest-cpp-reverse-proxy.conf.template
IrodsRestConf=${TEMPLATE_LOC}/irods_client_rest_cpp.json.template
copy_to_dst $IrodsRestConf /etc/irods
DESTFILE=$(copy_to_dst $RevProxyConf /etc/nginx/sites-available) &&\
    ln -s "$DESTFILE" /etc/nginx/sites-enabled
rm /etc/nginx/sites-enabled/default
service nginx restart
service  irods_client_rest_cpp start
