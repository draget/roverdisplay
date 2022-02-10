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
