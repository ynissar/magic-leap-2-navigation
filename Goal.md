# Overarching goal

To convert the hololens library in Hololens2-IRTRacking-main following the STTAR framework to a library that functions on the Magic Leap 2, an android based VR headset. 

## Features
* Easy-to-use tracking of retro-reflective marker arrays using the HoloLens 2 research mode
* Simultaneous tracking of multiple markers (tested with up to 5, can theoretically support a lot more)
* Support for partial marker occlusion - define marker arrays with 4 or more fiducials, keep tracking even when some are occluded (min. 3 visible at all times)
* Support for spherical markers and flat marker stickers
* Support for different marker types and sphere diameters to be tracked simultaneously
* Filter 3D fiducial world positions using Kalman Filters
* Filter marker array world position and rotation using low-pass filtering