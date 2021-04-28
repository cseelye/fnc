FROM centos:centos7.0.1406 as build

ARG BOOST_VERSION=76

# Get a newer set of kernel headers and dev toolchain
RUN yum --assumeyes install curl openssl && \
    rpm --import https://www.elrepo.org/RPM-GPG-KEY-elrepo.org && \
    yum --assumeyes install https://www.elrepo.org/elrepo-release-7.el7.elrepo.noarch.rpm && \
    yum --assumeyes remove systemd && \
    yum --assumeyes --enablerepo=elrepo-kernel install kernel-lt kernel-lt-headers kernel-lt-devel && \
    yum --assumeyes install centos-release-scl && \
    yum --assumeyes install devtoolset-9 glibc-static doxygen && \
    yum clean all

# Build a newer version of boost
RUN yum install --assumeyes libicu libicu-devel zlib-devel zlib python-devel bzip2-devel texinfo && \
    yum clean all && \
    cd /opt && \
    curl -LO https://dl.bintray.com/boostorg/release/1.${BOOST_VERSION}.0/source/boost_1_${BOOST_VERSION}_0.tar.gz && \
    tar xzf boost_1_${BOOST_VERSION}_0.tar.gz && \
    scl enable devtoolset-9 "bash -c 'cd /opt/boost_1_${BOOST_VERSION}_0 && ./bootstrap.sh --with-icu && ./b2 -d0 -a -j$(($(nproc)-1)) --layout=system'" && \
    rm boost_1_${BOOST_VERSION}_0.tar.gz

# Default to enabling the new toolchain
CMD ["/usr/bin/scl", "enable", "devtoolset-9", "bash"]


FROM scratch as runtime
COPY build/fnc /fnc
ENTRYPOINT ["/fnc"]
