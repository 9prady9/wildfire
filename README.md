wildfire
========
wildfire is a demo GUI application using [Qt5] and [ArrayFire] to show case image editing operations.

![demo](./data/demo.gif)

Edit Ops Added Till Date
------------------------
* Contrast modification
* Brightness modification
* Image translation
* Digital zoom
* Alpha blending
* Unsharpmask

Dependencies
------------
* [ArrayFire]
* [Qt5]

How to use?
-----------
```sh
git clone https://github.com/9prady9/wildfire
cd wildfire
```

Uncomment and update `AF_PATH` in `wildfire/wildfire.pro` to [ArrayFire] install path

```sh
mkdir -p build && cd build
qmake ../wildfire && make
```

[ArrayFire]: https://github.com/arrayfire/arrayfire
[Qt5]: http://qt-project.org/
