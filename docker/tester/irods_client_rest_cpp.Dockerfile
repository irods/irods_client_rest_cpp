FROM ubuntu:20.04

SHELL [ "/bin/bash", "-c" ]

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        apt-transport-https \
        gnupg \
        lsb-release \
        sudo \
        wget \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/*

RUN wget -qO - https://packages.irods.org/irods-signing-key.asc | apt-key add - && \
    echo "deb [arch=amd64] https://packages.irods.org/apt/ $(lsb_release -sc) main" | tee /etc/apt/sources.list.d/renci-irods.list

RUN apt-get update && \
    apt-get install -y \
        python3-pip \
        python3-pycurl \
        postgresql \
        unixodbc \
        libcurl4 \
        libkrb5-dev \
        irods-server \
        irods-database-plugin-postgres \
        irods-icommands \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/*

COPY db_commands.txt /

COPY irods_client_rest_cpp.json /tmp
RUN mkdir -p /etc/irods_client_rest_cpp && \
    cp /tmp/irods_client_rest_cpp.json /etc/irods_client_rest_cpp && \
    rm -f /tmp/irods_client_rest_cpp.json

COPY entrypoint.sh /
RUN chmod u+x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
