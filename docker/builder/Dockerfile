FROM ubuntu:20.04

SHELL [ "/bin/bash", "-c" ]

ENV DEBIAN_FRONTEND=noninteractive

# Make sure we're starting with an up-to-date image
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get autoremove -y --purge && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/*
# To mark all installed packages as manually installed:
#apt-mark showauto | xargs -r apt-mark manual

RUN apt-get update && \
    apt-get install -y \
        apt-transport-https \
        g++ \
        gcc \
        gnupg \
        lsb-release \
        lsof \
        make \
        ninja-build \
        python3 \
        python3-dev \
        python3-pip \
        python3-distro \
        python3-jsonschema \
        python3-packaging \
        python3-psutil \
        python3-pyodbc \
        python3-requests \
        sudo \
        super \
        wget \
    && \
    #pip3 --no-cache-dir install lief && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/*

RUN wget -qO - https://packages.irods.org/irods-signing-key.asc | apt-key add - && \
    echo "deb [arch=amd64] https://packages.irods.org/apt/ $(lsb_release -sc) main" | tee /etc/apt/sources.list.d/renci-irods.list && \
    wget -qO - https://core-dev.irods.org/irods-core-dev-signing-key.asc | apt-key add - && \
    echo "deb [arch=amd64] https://core-dev.irods.org/apt/ $(lsb_release -sc) main" | tee /etc/apt/sources.list.d/renci-irods-core-dev.list

RUN apt-get update && \
    apt-get install -y \
        irods-dev \
        irods-runtime \
        'irods-externals-boost*' \
        'irods-externals-json*' \
        'irods-externals-cmake*' \
        'irods-externals-clang*' \
        'irods-externals-pistache*' \
        'irods-externals-spdlog*' \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/*

ARG cmake_path="/opt/irods-externals/cmake3.21.4-0/bin"
ENV PATH=${cmake_path}:$PATH

ENV file_extension="deb"
ENV package_manager="apt-get"

COPY build_and_copy_packages_to_dir.sh /
RUN chmod u+x /build_and_copy_packages_to_dir.sh
ENTRYPOINT ["./build_and_copy_packages_to_dir.sh"]
