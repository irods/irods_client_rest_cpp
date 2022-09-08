FROM ubuntu:20.04

SHELL [ "/bin/bash", "-c" ]

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        apt-transport-https \
        gnupg \
        lsb-release \
        rsyslog \
        sudo \
        wget \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/*

RUN wget -qO - https://packages.irods.org/irods-signing-key.asc | apt-key add - && \
    echo "deb [arch=amd64] https://packages.irods.org/apt/ $(lsb_release -sc) main" | tee /etc/apt/sources.list.d/renci-irods.list

# Packages are installed at build time for the Docker image.
# This forces the builder to copy the built .deb package for the REST client to the local build context.
# The .deb file is removed from the image after it has been installed.
ARG local_package
COPY ${local_package} /tmp
RUN apt-get update && \
    (dpkg -i /tmp/${local_package} || true) && \
    apt-get install -fy --allow-downgrades && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/*

# Use the default rsyslog configuration for this image
RUN cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.conf.rsyslog /etc/rsyslog.d/00-irods_client_rest_cpp.conf && \
    cp /etc/irods_client_rest_cpp/irods_client_rest_cpp.logrotate /etc/logrotate.d/irods_client_rest_cpp

COPY entrypoint.sh /
RUN chmod u+x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
