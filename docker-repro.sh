#!/bin/bash
# Reproduz o ambiente CI (Ubuntu 24.04 + g++13) num contentor Docker.
# Monta o repo (com submodule JUCE), instala deps, compila VST3, corre pluginval.
# Uso: ./docker-repro.sh  (demora ~15-25 min a primeira vez)
set -e
IMG=deverb-ci-repro
docker build -t $IMG - <<'DOCKERFILE'
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
  build-essential cmake ninja-build pkg-config curl unzip gdb \
  libasound2-dev libjack-jackd2-dev ladspa-sdk \
  libcurl4-openssl-dev libfreetype6-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxrender-dev libxi-dev \
  libglu1-mesa-dev mesa-common-dev libegl-dev \
  && rm -rf /var/lib/apt/lists/*
RUN curl -sL -o /tmp/pv.zip https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Linux.zip \
  && mkdir -p /opt/pv && unzip -o -q /tmp/pv.zip -d /opt/pv && chmod +x /opt/pv/pluginval
WORKDIR /src
DOCKERFILE
echo "=== imagem pronta, a compilar VST3 ==="
docker run --rm -v /home/pedro/audio-dev/deVerb:/src $IMG bash -c "
  cmake -S /src -B /src/build-u24 -G Ninja -DCMAKE_BUILD_TYPE=Release -DDEVERB_FORMATS='Standalone;VST3' &&
  cmake --build /src/build-u24 --target deVerb_VST3 &&
  echo '=== pluginval ===' &&
  /opt/pv/pluginval --validate /src/build-u24/deVerb_artefacts/Release/VST3/deVerb.vst3 --strictness-level 5; echo \"pluginval exit: \$?\""
