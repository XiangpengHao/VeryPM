FROM haoxiangpeng/latest-cpp

COPY . /usr/src/cpp_project
WORKDIR /usr/src/cpp_project
RUN mkdir build_tmp && cd build_tmp && cmake -DCMAKE_BUILD_TYPE=Release -DCASCADE_LAKE=0 -DSKIP_BENCHMARK=1 .. && make -j4
ENTRYPOINT ["make", "-C", "build_tmp", "test" ]
