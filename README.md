wildfire
========

This repository is the base camp for an image editing application based on [ArrayFire](http://www.arrayfire.com/docs/) and [Qt5](http://qt-project.org/).

I am currently working on creating a GUI application using Qt that using ArrayFire as compute backend for performing image editing operations.

First phase will be do create functions using ArrayFire that does the necessray edit operation. Once the compute part is working fine, I shall proceed to the UI creation.

To-Date
-------
I have added a single c++ source file that has the initial set of functions that let me do the following operations.

* Contrast modification
* Brightness modification
* Image translation
* Digital zoom
* Alpha blending
* Unsharpmask
