from debian:bookworm-slim as builder

run apt-get update &&\
    apt-get install -y\
    build-essential \
    cmake \
    git \
    wget \
    autoconf \
    automake \
    libtool \
    pkg-config \
    libavformat-dev \
    libavcodec-dev \
    libavfilter-dev \
    libavutil-dev \
    libswresample-dev \
    && rm -rf /var/lib/apt/lists/*

workdir /deps

run git clone https://github.com/dpirch/libfvad.git /tmp/libfvad && \
        cd /tmp/libfvad && \
        autoreconf -i && \
        ./configure && \
        make && make install && \
        ldconfig

workdir /build

copy . .

run cmake . && cmake --build . && ls -la /build/*.so

from alpine:3.19
workdir /output

copy --from=builder /build/libpodcast_silence_remover.so .

cmd cp libpodcast_silence_remover.so /shared_output
