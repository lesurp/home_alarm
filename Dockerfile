FROM alpine AS builder 

RUN apk -U upgrade
RUN apk add musl curl ca-certificates cmake ninja linux-headers pkgconfig git flac-dev openal-soft-dev libogg-dev ffmpeg gcc py3-pip python3 g++ sfml 
RUN pip3 install meson

RUN curl -Ls https://github.com/opencv/opencv/archive/4.5.0.tar.gz | tar xvz -C /
WORKDIR /opencv-4.5.0/build
RUN cmake .. \
    -G Ninja \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_opencv_apps=OFF \
    -DBUILD_opencv_calib3d=OFF \
    -DBUILD_opencv_dnn=OFF \
    -DBUILD_opencv_features2d=OFF \
    -DBUILD_opencv_flann=OFF \
    -DBUILD_opencv_gapi=OFF \
    -DBUILD_opencv_highgui=OFF \
    -DBUILD_opencv_ml=OFF \
    -DBUILD_opencv_objdetect=OFF \
    -DBUILD_opencv_photo=OFF \
    -DBUILD_opencv_python2=OFF \
    -DBUILD_opencv_python3=OFF \
    -DBUILD_opencv_sitching=OFF \
    -DBUILD_opencv_video=OFF \
    -DWITH_CUDA=OFF \
    -DWITH_OPENCL=OFF \
    -DWITH_PROTOBUF=OFF \
    -DBUILD_PROTOBUF=OFF \
    -DWITH_GTK=OFF \
    -DWITH_OPENGL=OFF \
    -DWITH_OPENGL=OFF \
    -DBUILD_JAVA=OFF \
    -DWITH_1394=OFF \
    -DWITH_VTK=OFF \
    -DWITH_EIGEN=OFF \
    -DWITH_GSTREAMER=OFF \
    -DWITH_JASPER=OFF \
    -DWITH_OPENCLAMDFFT=OFF \
    -DWITH_OPENCLAMDBLAS=OFF \
    -DWITH_LAPACK=OFF \
    -DWITH_FFMPEG=OFF \
    -DOPENCV_FFMPEG_SKIP_BUILD_CHECK=ON \
    -DENABLE_CONFIG_VERIFICATION=ON \
    -DOPENCV_GENERATE_PKGCONFIG=ON
RUN ninja install
ENV PKG_CONFIG_PATH /opencv-4.5.0/build/unix-install/:${PKG_CONFIG_PATH}

RUN curl -Ls https://github.com/xiph/vorbis/archive/v1.3.7.tar.gz | tar xvz -C /
WORKDIR /vorbis-1.3.7/build
RUN cmake .. \
    -G Ninja \
    -DBUILD_SHARED_LIBS=OFF
RUN ninja install
ENV PKG_CONFIG_PATH /vorbis-1.3.7/build:${PKG_CONFIG_PATH}


RUN curl -Ls https://github.com/SFML/SFML/archive/2.5.1.tar.gz | tar xvz -C /
WORKDIR /SFML-2.5.1/build
RUN cmake .. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DSFML_BUILD_EXAMPLES=OFF \
    -DSFML_BUILD_GRAPHICS=OFF \
    -DSFML_BUILD_WINDOW=OFF \
    -DSFML_BUILD_NETWORK=OFF \
    -DSFML_USE_SYSTEM_DEPS=OFF \
    -DSFML_INSTALL_PKGCONFIG_FILES=ON \
    -DSFML_BUILD_AUDIO=ON
RUN ninja install
ENV PKG_CONFIG_PATH /SFML-2.5.1/build/tools/pkg-config:${PKG_CONFIG_PATH}

# Fix broken pkgconfig files...
RUN sed 's/-lvorbis/-l:libvorbis.a/' -i /vorbis-1.3.7/build/vorbis.pc
RUN sed 's/-lvorbisenc/-l:libvorbisenc.a/' -i /vorbis-1.3.7/build/vorbisenc.pc
RUN sed 's/-lvorbisfile/-l:libvorbisfile.a/' -i /vorbis-1.3.7/build/vorbisfile.pc
RUN sed 's/-lsfml-audio/-l:libsfml-audio-s.a/' -i /SFML-2.5.1/build/tools/pkg-config/sfml-audio.pc
RUN sed 's/-lsfml-system/-l:libsfml-system-s.a/' -i /SFML-2.5.1/build/tools/pkg-config/sfml-system.pc

WORKDIR /build_root
COPY meson_options.txt meson.build .
COPY src ./src
COPY subprojects/packagefiles ./subprojects/packagefiles
COPY subprojects/*.wrap ./subprojects
ENV LDFLAGS=-static-libstdc++
RUN meson setup build -Dstatic_linking=true
WORKDIR /build_root/build/
RUN ninja
 
FROM alpine
RUN apk -U upgrade && apk add musl flac-dev openal-soft-dev
COPY --from=builder /build_root/build/detection /opt/burglar_fucker/
COPY resources /opt/burglar_fucker/resources
COPY burglar_fucker_conf.toml.aarch64 /opt/burglar_fucker/burglar_fucker_conf.toml
WORKDIR /opt/burglar_fucker/
ENTRYPOINT ["./detection"]
