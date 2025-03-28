#syntax=docker/dockerfile:1

# --platform=linux/386

FROM debian:bookworm AS build
ARG NCPU=1

RUN apt-get update && apt-get install -y make gcc bzip2 zlib1g-dev

# expand Runix toolchain (with binutils 2.9)
ADD vendors/Sony/PlayStation1/PS1Linux/tools/mipsel-linux-cross-i586.tar.gz /

# overwrite binutils to Runix toolchain (binutils only, no gcc. 2.11 supports emit-reloc that is used by ld-elf2flt)
ADD --checksum=sha256:3911d9669926774f0cdc8cf4813771fba6e752dc3bcaae879ca7b52516a721fa https://sourceware.org/pub/binutils/releases/binutils-2.11.2a.tar.bz2 /binutils-2.11.2a.tar.bz2
RUN tar xf binutils-2.11.2a.tar.bz2 && mkdir binutils-2.11.2/build
RUN cd binutils-2.11.2/build && CFLAGS="-Wno-error=implicit-fallthrough -Wno-error=unused-value -Wno-error=cast-function-type -Wno-error=format-overflow -Wno-error=pointer-compare -Wno-error=shift-negative-value" ../configure --target=mipsel-linux --enable-targets=mipsel-elf
# build POTFILES.in without -j first for "tmp"-race workaround
RUN cd binutils-2.11.2/build && make -C bfd po/SRC-POTFILES.in po/BLD-POTFILES.in && make -j$NCPU && make install

# binutils-2.22 just for libiberty suitable for elf2flt... (2.11.2/2.15 lacks pex_*, bookworm's libiberty-dev:20230104-1 lacks CONST/PARAMS of ansidecl.h)
# 2.22 itself is unusable for uClinux; -mcpu=r3000 not recognized
ADD --checksum=sha256:6c7af8ed1c8cf9b4b9d6e6fe09a3e1d3d479fe63984ba8b9b26bf356b6313ca9 https://sourceware.org/pub/binutils/releases/binutils-2.22.tar.bz2 /binutils-2.22.tar.bz2
RUN tar xf binutils-2.22.tar.bz2 && mkdir binutils-2.22/build
RUN cd binutils-2.22/build && CFLAGS="-Wno-error=implicit-fallthrough -Wno-error=unused-value -Wno-error=cast-function-type -Wno-error=format-overflow -Wno-error=pointer-compare -Wno-error=shift-negative-value" ../configure --target=mipsel-linux --enable-targets=mipsel-elf
RUN cd binutils-2.22/build && make -j$NCPU

# add elf2flt (master@2025-03-20)
ADD https://github.com/murachue/elf2flt.git#7c318f5692fa293494d537d55ad236ec1923d5e8 /elf2flt
RUN cd elf2flt && ./configure --target=mipsel-linux --with-binutils-build-dir=../binutils-2.22/build --with-binutils-include-dir=../binutils-2.22/include
RUN cd elf2flt && make -j$NCPU && make install

FROM debian:bookworm AS provision
RUN rm -rf /usr/local/*
COPY --from=build /usr/local /usr/local/
RUN apt-get update && apt-get install -y make gcc libncurses-dev git  less vim-tiny
RUN apt-get clean

FROM scratch
COPY --from=provision / /
WORKDIR /work
CMD ["bash"]
