FROM nvidia/cuda:10.2-cudnn8-devel-ubuntu18.04

RUN apt-get update \
	&& apt-get install -y git cmake g++ libglew-dev \
&& mkdir /tmp_build && cd /tmp_build \
&& git clone https://github.com/opencv/opencv.git \
&& git clone https://github.com/opencv/opencv_contrib \
&& cd opencv_contrib \
&& git checkout 3.4.3  \
&& cd ../opencv \
&& git checkout 3.4.3  \
&& mkdir build && cd build \
&& cmake  -DWITH_PTHREADS_PF=OFF   -DCMAKE_CONFIGURATION_TYPES=Release -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules -D WITH_CUDA=on ..  \
&& make -j5 && make install \
&& echo "deb http://packages.ros.org/ros/ubuntu bionic main" > /etc/apt/sources.list.d/ros-latest.list \
&& cd ../.. \
&& apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654 \
&& apt update \
&& DEBIAN_FRONTEND=noninteractive apt-get -y install tzdata  \
&& apt-get install -y ros-melodic-desktop-full \
&& source /opt/ros/melodic/setup.bash \
&& apt install python-rosdep python-rosinstall python-rosinstall-generator python-wstool build-essential \
&& rosdep init && rosdep update \
&& git clone https://github.com/stevenlovegrove/Pangolin.git \
&& cd Pangolin && mkdir build && cd build && cmake .. && make -j8 \
&& cd ../.. \
&& git clone https://github.com/xaldyz/dataflow-orbslam.git \
&& cd dataflow-orbslam \
&& cd Thirdparty/DBoW2 \
&& mkdir build \
&& cd build \
&& cmake .. -DCMAKE_BUILD_TYPE=Release \
&& make -j \
&& cd ../../g2o \
&& echo "Configuring and building Thirdparty/g2o ..." \
&& mkdir build \
&& cd build \
&& cmake .. -DCMAKE_BUILD_TYPE=Release \
&& make -j \
&& cd ../../../ \
&& echo "Uncompress vocabulary ..." \
&& cd Vocabulary \
&& tar -xf ORBvoc.txt.tar.gz \
&& cd .. \
&& mkdir build && cd build \
&& cmake -D USE_CUSTOM_VX=true USE_PIPELINE=FALSE -DCMAKE_BUILD_TYPE=Release .. \
&& make -j4 \
&& cd .. && ln -s lib/_custom_gpu_wait_0/libORB_SLAM2.so lib/ \
&& cd Examples/ROS/ && mkdir src && mv ORB_SLAM2 src
&& echo "#!/bin/bash" > /ros_entrypoint.sh \
&& echo "set -e" >> /ros_entrypoint.sh \
&& echo "" >> /ros_entrypoint.sh \
&& echo "# setup ros environment" >> /ros_entrypoint.sh \
&& echo 'source "/opt/ros/melodic/setup.bash"' >> /ros_entrypoint.sh \
&& echo 'export ROS_PACKAGE_PATH=/tmp_build/dataflow-orbslam/Examples/ROS/:${ROS_PACKAGE_PATH}' >> /ros_entrypoint.sh \
&& echo 'exec "$@"' >> /ros_entrypoint.sh 

ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["bash"]