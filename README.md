# [Jetpack 4.3 update]

Added support for OpenCV 4 that comes with Jetpack 4.3.

You need to build OpenCV with the opencv-contrib modules enabed. [Here is a script to install OpenCV 4 with opencv-contrib enabled on Jetson board](https://github.com/AastaNV/JEP/blob/master/script/install_opencv4.1.1_Jetson.sh).

# Dataflow ORB-SLAM2

**Authors**: Stefano Aldegheri, based upon [Raul Mur-Artal](http://webdiis.unizar.es/~raulmur/) ([ORB-SLAM2](https://github.com/raulmur/ORB_SLAM2))

This implementation aims to target better efficiency in the feature extraction part using a dataflow description of the algorithm and using pipelining. These enhancement ensure a real-time implementation in the embedded Jetson TX2 board, previously unachievable.

⚠️ At the moment, only the monocular version of the KITTI dataset is supported.

> **Note:** The library is intended to be built & run on **NVIDIA Jetson TX2** with **JetPack 4.2.2**, but it should works fine with the newer versions as well.

### Related Publications:

Stefano Aldegheri, Nicola Bombieri, Daniele D. Bloisi and Alessandro Farinelli. **Data Flow ORB-SLAM for Real-time Performance
on Embedded GPU Boards**. *IEEE/RSJ International Conference on Intelligent Robots and Systems*. **[PDF](https://www.dropbox.com/s/p3bh0lfi5ahe28e/IROS2019.pdf?dl=0)**.

## 1. Setting up NVIDIA Jetson with JetPack

NVIDIA [JetPack](https://developer.nvidia.com/embedded/jetpack) is a comprehensive SDK for Jetson for both developing and deploying AI and computer vision applications. JetPack simplifies installation of the OS and drivers and contains the following components:

- L4T Kernel / BSP
- CUDA Toolkit
- cuDNN
- TensorRT
- OpenCV
- VisionWorks
- Multimedia API's

Jetson TX2 should be flashed by downloading the [NVIDIA SDK Manager](https://developer.nvidia.com/nvsdk-manager) to a host PC running Ubuntu 16.04 x86_64 or Ubuntu 18.04 x86_64. 
For more details, please refer to the [NVIDIA SDK Manager Documentation](https://docs.nvidia.com/sdk-manager/install-with-sdkm-jetson/index.html).

## 2. Installing dependencies
### Pangolin
Pangolin is used for visualization and user interface. Download and install instructions can be found at: https://github.com/stevenlovegrove/Pangolin.

### Eigen3
Required by g2o (see below). Download and install instructions can be found at: http://eigen.tuxfamily.org. Required at least 3.1.0.

### DBoW2 and g2o (Included in Thirdparty folder)
We use modified versions of the [DBoW2](https://github.com/dorian3d/DBoW2) library to perform place recognition and [g2o](https://github.com/RainerKuemmerle/g2o) library to perform non-linear optimizations. Both modified libraries (which are BSD) are included in the Thirdparty folder.

## 3. Building the Project from Source
To download the code, navigate to a folder of your choosing on the Jetson (we take as reference the $HOME folder). First, make sure git and cmake are installed:
```
$ sudo apt-get update
$ sudo apt-get install git cmake
```

Then clone the dataflow-orbslam project:
```
$ git clone https://github.com/xaldyz/dataflow-orbslam.git
```

Build DBoW and g2o modified libraries in **Thirdparty** folder
```
$ cd ~/dataflow-orbslam/Thirdparty/DBoW2	# Build DBoW
$ mkdir build && cd build
$ cmake ..
$ make
$ cd ~/dataflow-orbslam/Thirdparty/g2o		# Build g2o
$ mkdir build && cd build
$ cmake ..
$ make
```

And finally you can build dataflow-orbslam
```
$ cd ~/dataflow-orbslam
$ mkdir build && cd build
$ cmake ..
$ make
```
⚠️ The project is set to build only the Monocular mono_kitti example by default.

> **Note:** In the CMakeLists.txt file of the project folder you can set up CUSTOM_VX and PIPELINE variables to switch ON or OFF these optimisations.

## 4. Run some Examples

First of all you need to download some example sequences from http://www.cvlibs.net/datasets/kitti/eval_odometry.php. We tested the application on the sequences 03, 04, 05 and 06 from [grayscale odometry dataset](http://www.cvlibs.net/download.php?file=data_odometry_gray.zip).


Then uncompress the Vocabulary:
```
$ cd ~/dataflow-orbslam/Vocabulary
$ tar -zxvf ORBvoc.txt.tar.gz
```

### Run the Mono Kitti example
From the project folder go into the *build* subfolder and run the follow command:

```
./mono_kitti ../Vocabulary/ORBvoc.txt ../Examples/Monocular/KITTI03.yaml PATH_TO_SEQUENCE_FOLDER
```

If all works fine you should see the execution below:

⚠️ Edit parameters on the .yaml file

> **Note:** Only the KITTI04-12.yaml in the Examples subfolder is expected to run. This is because there are two parameters that must be set to 
> - Camera.width: the width of the image (different KITTI streams has different widths)
> - Camera.height: the height of the image (different KITTI streams has different heights)
