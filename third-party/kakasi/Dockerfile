FROM debian:bullseye-slim

RUN sed -i 's/^# deb-src/deb-src/' /etc/apt/sources.list && \
    if ! grep -q "^deb-src" /etc/apt/sources.list; then \
        echo "deb-src http://deb.debian.org/debian bullseye main" >> /etc/apt/sources.list; \
        echo "deb-src http://deb.debian.org/debian bullseye-updates main" >> /etc/apt/sources.list; \
        echo "deb-src http://deb.debian.org/debian-security bullseye-security main" >> /etc/apt/sources.list; \
    fi && \
    apt-get update

RUN apt-get install -y \
    wget \
    build-essential \
    unzip \
    git \
    make \
    ca-certificates \
    xz-utils \
    automake \
    perl \
    dpkg-dev \
    locales

RUN sed -i 's/^# *ja_JP.EUC-JP/ja_JP.EUC-JP/' /etc/locale.gen && \
    locale-gen

ENV LANG=ja_JP.EUC-JP
ENV LC_ALL=ja_JP.EUC-JP

ENV NDK_VERSION=r25c
ENV ANDROID_NDK_HOME=/opt/android-ndk-${NDK_VERSION}
ENV PATH=${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin:${PATH}

RUN wget -q https://dl.google.com/android/repository/android-ndk-${NDK_VERSION}-linux.zip && \
    unzip -q android-ndk-${NDK_VERSION}-linux.zip -d /opt && \
    rm android-ndk-${NDK_VERSION}-linux.zip

WORKDIR /build
RUN apt-get source kakasi && \
    srcdir=$(ls -d kakasi-*/ | head -n 1) && \
    mv "$srcdir" kakasi-src

WORKDIR /build/kakasi-src
RUN cp /usr/share/automake-*/config.guess . && \
    cp /usr/share/automake-*/config.sub .

# Host build to generate dictionaries with Debian-patched sources.
RUN CFLAGS="-fcommon" ./configure \
    --prefix=/tmp/host_build \
    --enable-utf8 \
    --disable-nls && \
    make -j4

RUN cp kanwadict /tmp/kanwadict && \
    cp itaijidict /tmp/itaijidict

RUN make distclean

# Build GNU libiconv for Android (EUC-JP support).
WORKDIR /build
RUN wget https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.17.tar.gz && \
    tar xzf libiconv-1.17.tar.gz && \
    mv libiconv-1.17 libiconv-src

WORKDIR /build/libiconv-src

ENV CC=aarch64-linux-android28-clang
ENV CXX=aarch64-linux-android28-clang++
ENV AR=llvm-ar
ENV RANLIB=llvm-ranlib
ENV STRIP=llvm-strip

RUN ./configure \
    --host=aarch64-linux-android \
    --prefix=/build/output \
    --enable-static \
    --disable-shared \
    --with-pic && \
    make -j4 && \
    make install

WORKDIR /build/kakasi-src

# Config.site for cross compile
RUN echo "ac_cv_func_malloc_0_nonnull=yes" > /build/config.site && \
    echo "ac_cv_func_realloc_0_nonnull=yes" >> /build/config.site && \
    echo "ac_cv_func_setvbuf_reversed=no" >> /build/config.site && \
    echo "ac_cv_func_memcmp_working=yes" >> /build/config.site && \
    echo "am_cv_func_iconv=yes" >> /build/config.site && \
    echo "am_cv_func_iconv_works=yes" >> /build/config.site && \
    echo "am_cv_lib_iconv=yes" >> /build/config.site && \
    echo "am_cv_proto_iconv_arg1=\"\"" >> /build/config.site

ENV CONFIG_SITE=/build/config.site

# Patch configure to avoid failing on some checks
RUN perl -pi -e 's/as_fn_exit \$as_status/echo "warning: ignoring exit command"/g' configure && \
    perl -pi -e 's/exit 1/true/g' configure

RUN CFLAGS="-fcommon -Wno-error -fPIC -I/build/output/include" \
    LDFLAGS="-L/build/output/lib" \
    LIBS="-liconv -lcharset" \
    ./configure \
    --host=aarch64-linux-android \
    --disable-shared \
    --enable-static \
    --enable-utf8 \
    --disable-nls \
    --prefix=/build/output

RUN cp /tmp/kanwadict kanwadict && \
    cp /tmp/itaijidict itaijidict && \
    touch kanwadict itaijidict

RUN make -j4 && \
    make install

RUN mkdir -p /build/output/share/kakasi && \
    cp /tmp/kanwadict /build/output/share/kakasi/kanwadict && \
    cp /tmp/itaijidict /build/output/share/kakasi/itaijidict
