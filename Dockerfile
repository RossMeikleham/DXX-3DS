from devkitpro/devkitarm:latest

RUN apt-get update && apt-get install -y gcc g++ zip cmake

# Install makerom
RUN cd / && git clone https://github.com/profi200/Project_CTR
RUN cd /Project_CTR/makerom && make
ENV PATH="/Project_CTR/makerom/:${PATH}"

# Install bannertool
RUN cd / && git clone https://github.com/Steveice10/bannertool
RUN cd /bannertool && git submodule update --init --recursive && make 
ENV PATH="/bannertool/output/linux-x86_64/:${PATH}"

# Install PHYSFS
RUN cd / && git clone https://github.com/RossMeikleham/physfs-3ds
RUN cd physfs-3ds && mkdir build && cd build &&\
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../Toolchain.cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$DEVKITPRO/portlibs/3ds \
        -DPHYSFS_BUILD_SHARED=OFF -DPHYSFS_BUILD_TEST=OFF &&\
    make &&\
    make install

# Build D1X & D2X .3dsx and .cia files
RUN mkdir /dxx/
ADD  d1 /dxx/d1/
ADD  d2 /dxx/d2/
WORKDIR /dxx/

CMD cd d1 && make && cp d1x-3ds.3dsx /mnt &&\ 
    cd 3ds_data && ./make_cia.sh && cp d1x-3ds.cia /mnt &&\
    cd ../../d2 && make && cp d2x-3ds.3dsx /mnt &&\
    cd 3ds_data && ./make_cia.sh && cp d2x-3ds.cia /mnt
