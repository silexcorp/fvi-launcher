#! /bin/bash

set -o errexit
set -o nounset

if [[ -z ${QT_VER-} || -z ${TARGET-} ]]; then
  echo "Please define QT_VER and TARGET first"
  exit 1
fi

set -o xtrace


# Native dependencies

if [[ $TARGET = x11* ]]; then
  sudo apt-add-repository -y ppa:brightbox/ruby-ng
  sudo apt-get -qq update
  sudo apt-get install -y \
    libasound-dev \
    libgl1-mesa-dev \
    libgstreamer-plugins-base1.0-dev \
    libpulse-dev \
    libudev-dev \
    libxi-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libsdl2-dev \
    ruby2.4
  gem install fpm -v 1.10.2
fi

# Install the toolchain

TOOLS_URL=https://github.com/Warfork/fvi-toolchain/raw/master

pushd /tmp
  wget ${TOOLS_URL}/qt${QT_VER//./}_${TARGET}.txz

  if [[ $TARGET == macos* ]]; then OUTDIR=/usr/local; else OUTDIR=/opt; fi
  for f in *.txz; do sudo tar xJf ${f} -C ${OUTDIR}/; done
popd
