# RoverDisplay

## Setup

This project depends upon [libcomm14cux](https://github.com/colinbourassa/libcomm14cux).

```
rover
|--- libcomm14cux
|    |--- src
|    |--- build
|--- roverdisplay
     |--- build
```


## Building

```
mkdir rover && cd rover
git clone https://github.com/draget/roverdisplay
git clone https://github.com/colinbourassa/libcomm14cux
```
```
cd libcomm14cux
mkdir build && cd build
cmake ..
make
cd ..
```
```
cd roverdisplay
mkdir build && cd build
cmake ..
make
```

## Cross-compiling

The provided toolchain file can be used to build for ARMv3, which is perfect for a [Psion 5mx](https://en.wikipedia.org/wiki/Psion_Series_5). Note ARMv3 was dropped in GCC 9, but you could find an older version somewhere like [here](https://download.atmark-techno.com/debian/sarge/). Of course the toolchain file is easily adapted for other architectures.

Remember you must use it when building libcomm14cux too.

```
cmake -DCMAKE_TOOLCHAIN_FILE=../TC-arm.cmake ..
```
