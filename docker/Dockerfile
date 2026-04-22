# syntax=docker/dockerfile:1.6
#
# Lunar Studio — reproducible from-source build
# ---------------------------------------------------------------------------
# Builds ISIS3 (with KPLO LUTI camera model + modified campt/CameraPointInfo)
# from the Lunar Studio ISIS fork and installs the Ames Stereo Pipeline.
#
# Build context expects the Lunar Studio ISIS fork available at the path
# given by ISIS_SRC_DIR (default: ./isis_src). Provide it by either:
#   1. Cloning: `git clone https://github.com/Suwan00/lunar_studio
#       docker/isis_src` before `docker build`, or
#   2. Passing a bind mount: `docker build --build-context
#       isis_src=/path/to/lunar_studio ...` and setting ISIS_SRC_DIR=.
# The supplied docker/build_image.sh automates option 1.
# ---------------------------------------------------------------------------

ARG BASE_IMAGE=continuumio/miniconda3:24.11.1-0
FROM ${BASE_IMAGE}

LABEL org.opencontainers.image.title="Lunar Studio"
LABEL org.opencontainers.image.description="ISIS3-based lunar data preprocessing pipeline with KPLO LUTI camera model and LNEM-compatible campt"
LABEL org.opencontainers.image.source="https://github.com/Suwan00/lunar_studio"
LABEL org.opencontainers.image.licenses="Public Domain (ISIS3) + MIT (Lunar Studio additions)"

SHELL ["/bin/bash", "-lc"]

# ---------------------------------------------------------------------------
# System dependencies (build tools + ISIS3 / Qt / GL runtime libraries)
# ---------------------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git wget curl bzip2 vim locate \
        ca-certificates \
        libgl1-mesa-glx libglib2.0-0 libsm6 libxext6 libxrender1 libgomp1 \
        libxcb-xinerama0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
        libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-xfixes0 \
        libxkbcommon-x11-0 libdbus-1-3 \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && updatedb

# ---------------------------------------------------------------------------
# Conda environment: lunar_studio (ISIS3 deps + Python pipeline deps)
#   environment.yml is vendored into the docker/ folder; see docker/sync_env.sh
# ---------------------------------------------------------------------------
COPY environment.yml /tmp/environment.yml
RUN conda config --set always_yes true \
    && conda update -n base -c defaults conda \
    && conda env create -f /tmp/environment.yml \
    && conda clean -afy \
    && rm /tmp/environment.yml

ENV CONDA_DEFAULT_ENV=lunar_studio
ENV CONDA_PREFIX=/opt/conda/envs/lunar_studio
ENV PATH=/opt/conda/envs/lunar_studio/bin:/opt/conda/bin:${PATH}

# ---------------------------------------------------------------------------
# ISIS3 fork source (Lunar Studio additions: KPLO LUTI + campt modifications)
# Injected via BuildKit named build context `isis_src` — see build_image.sh.
# ---------------------------------------------------------------------------
ENV ISIS3_ROOT=/opt/ISIS3
RUN mkdir -p ${ISIS3_ROOT}
COPY --from=isis_src . ${ISIS3_ROOT}/

# ---------------------------------------------------------------------------
# Build ISIS3 with ninja
#   - Builds all apps (campt, jigsaw, lronac2isis, lronaccal, lronacecho,
#     spiceinit, footprintinit, seedgrid, cnetadd, cnetedit, pointreg,
#     findfeatures, crop, camrange, isis2std) plus the LUTI camera model.
#   - Final install prefix: /opt/ISIS3/build (matches existing Lunar Studio
#     image layout so downstream scripts keep working).
# ---------------------------------------------------------------------------
RUN --mount=type=cache,target=/opt/ccache \
    source /opt/conda/etc/profile.d/conda.sh && conda activate lunar_studio \
    && mkdir -p ${ISIS3_ROOT}/build \
    && cd ${ISIS3_ROOT}/build \
    && cmake -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${ISIS3_ROOT}/build \
        -DJP2KFLAG=OFF \
        -Dpybindings=OFF \
        -DbuildTests=OFF \
        ../isis \
    && ninja -j"$(nproc)" install \
    && find ${ISIS3_ROOT}/build -name "*.o" -delete

# Ensure the conda-env campt (with our modifications) shadows any leftover
# binary in /opt/ISIS3/build/bin, matching existing image behaviour.
RUN cp -f /opt/conda/envs/lunar_studio/bin/campt ${ISIS3_ROOT}/build/bin/campt || true

# XML symlinks for app help strings
RUN mkdir -p ${ISIS3_ROOT}/build/bin/xml \
    && cd ${ISIS3_ROOT}/build/bin/xml \
    && find ${ISIS3_ROOT}/isis/src -name "*.xml" -path "*/apps/*" -type f | while read xml_file; do \
            app_name=$(basename "$xml_file" .xml); \
            ln -sf "$xml_file" "${app_name}.xml"; \
        done \
    && echo "Linked $(ls -1 ${ISIS3_ROOT}/build/bin/xml/*.xml | wc -l) app XML help files"

# IsisPreferences
RUN if [ -f "${ISIS3_ROOT}/isis/IsisPreferences" ]; then \
        cp -f ${ISIS3_ROOT}/isis/IsisPreferences ${ISIS3_ROOT}/build/IsisPreferences; \
        cp -f ${ISIS3_ROOT}/isis/IsisPreferences /opt/conda/envs/lunar_studio/IsisPreferences; \
    fi

# Dynamic linker paths
RUN echo "/opt/conda/envs/lunar_studio/lib" > /etc/ld.so.conf.d/isis.conf \
    && echo "${ISIS3_ROOT}/build/lib" >> /etc/ld.so.conf.d/isis.conf \
    && ldconfig

# ---------------------------------------------------------------------------
# Conda activate/deactivate hooks: set up ISIS env vars on env activation
# ---------------------------------------------------------------------------
RUN mkdir -p /opt/conda/envs/lunar_studio/etc/conda/activate.d \
             /opt/conda/envs/lunar_studio/etc/conda/deactivate.d

RUN cat > /opt/conda/envs/lunar_studio/etc/conda/activate.d/isis_env.sh <<'EOF'
#!/bin/bash
# ISIS3 environment variables
export ISISROOT=/opt/ISIS3/build
export ISIS3DATA=/opt/ISIS3/isis/data

# ISISDATA — honours DOCKER_ISISDATA (set by entrypoint) or host-injected value
if [ -n "${DOCKER_ISISDATA}" ]; then
    export ISISDATA="${DOCKER_ISISDATA}"
elif [ -z "${ISISDATA}" ]; then
    export ISISDATA=/workspace
fi

# PATH: conda env first, then ISIS build bin
if [[ ":$PATH:" != *":/opt/conda/envs/lunar_studio/bin:"* ]]; then
    export PATH="/opt/conda/envs/lunar_studio/bin:$PATH"
fi
if [[ ":$PATH:" != *":/opt/ISIS3/build/bin:"* ]]; then
    export PATH="$PATH:/opt/ISIS3/build/bin"
fi

# Libraries
if [[ ":$LD_LIBRARY_PATH:" != *":/opt/conda/envs/lunar_studio/lib:"* ]]; then
    export LD_LIBRARY_PATH="/opt/conda/envs/lunar_studio/lib:${LD_LIBRARY_PATH}"
fi
if [[ ":$LD_LIBRARY_PATH:" != *":/opt/ISIS3/build/lib:"* ]]; then
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/opt/ISIS3/build/lib"
fi

export QT_PLUGIN_PATH=/opt/conda/envs/lunar_studio/plugins
EOF

RUN cat > /opt/conda/envs/lunar_studio/etc/conda/deactivate.d/isis_env.sh <<'EOF'
#!/bin/bash
export PATH=$(echo "$PATH" \
    | sed -e "s|/opt/ISIS3/build/bin:||g" \
          -e "s|:/opt/ISIS3/build/bin||g" \
          -e "s|/opt/conda/envs/lunar_studio/bin:||g")
export LD_LIBRARY_PATH=$(echo "$LD_LIBRARY_PATH" \
    | sed -e "s|/opt/ISIS3/build/lib:||g" \
          -e "s|:/opt/ISIS3/build/lib||g" \
          -e "s|/opt/conda/envs/lunar_studio/lib:||g")
unset ISISROOT ISIS3DATA QT_PLUGIN_PATH
EOF

RUN chmod +x /opt/conda/envs/lunar_studio/etc/conda/activate.d/isis_env.sh \
             /opt/conda/envs/lunar_studio/etc/conda/deactivate.d/isis_env.sh

# ---------------------------------------------------------------------------
# Ames Stereo Pipeline 3.5.0 (bundled for downstream ASP workflows)
# ---------------------------------------------------------------------------
ARG ASP_VERSION=3.5.0-2025-04-28
ARG ASP_URL=https://github.com/NeoGeographyToolkit/StereoPipeline/releases/download/3.5.0/StereoPipeline-${ASP_VERSION}-x86_64-Linux.tar.bz2
RUN cd /opt \
    && wget -q "${ASP_URL}" -O StereoPipeline.tar.bz2 \
    && tar --no-same-owner -xjf StereoPipeline.tar.bz2 \
    && rm StereoPipeline.tar.bz2

# ---------------------------------------------------------------------------
# Entrypoint: auto-activate lunar_studio, forward ISISDATA from host env
# ---------------------------------------------------------------------------
RUN cat > /usr/local/bin/entrypoint.sh <<'EOF'
#!/bin/bash
if [ -n "$ISISDATA" ]; then
    export DOCKER_ISISDATA="$ISISDATA"
fi
source /opt/conda/etc/profile.d/conda.sh
conda activate lunar_studio
if [ $# -eq 0 ]; then
    exec /bin/bash
else
    exec "$@"
fi
EOF
RUN chmod +x /usr/local/bin/entrypoint.sh

# Interactive-shell auto-activation
RUN echo '#!/bin/bash' > /root/.bashrc \
    && echo 'source /opt/conda/etc/profile.d/conda.sh' >> /root/.bashrc \
    && echo 'conda activate lunar_studio' >> /root/.bashrc \
    && echo 'export PATH="$PATH:/opt/StereoPipeline-'${ASP_VERSION}'-x86_64-Linux/bin"' >> /root/.bashrc

ENV SHELL=/bin/bash
WORKDIR /workspace
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD []
