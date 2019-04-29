# This is an auto generated Dockerfile for ros:ros-core
# generated from docker_images/create_ros_core_image.Dockerfile.em
FROM ubuntu:bionic

# setup timezone
RUN echo 'Etc/UTC' > /etc/timezone && \
    ln -s /usr/share/zoneinfo/Etc/UTC /etc/localtime && \
    apt-get update && apt-get install -q -y tzdata && rm -rf /var/lib/apt/lists/*

#Install ATI Radeon
RUN apt-get update --fix-missing && \
    apt-get install -y --no-install-recommends \
    libgl1-mesa-glx libgl1-mesa-dri

# install packages
RUN apt-get update && apt-get install -q -y \
    dirmngr \
    gnupg2 \
    lsb-release \
    && rm -rf /var/lib/apt/lists/*

# setup keys
RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 421C365BD9FF1F717815A3895523BAEEB01FA116

# setup sources.list
RUN echo "deb http://packages.ros.org/ros/ubuntu `lsb_release -sc` main" > /etc/apt/sources.list.d/ros-latest.list

# install bootstrap tools
RUN apt-get update && apt-get install --no-install-recommends -y \
    python-rosdep \
    python-rosinstall-generator \
    python-rosinstall \
    python-vcstools \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# setup environment
ENV LANG C.UTF-8
ENV LC_ALL C.UTF-8

# install ros packages
ENV ROS_DISTRO melodic
RUN apt-get update && apt-get install -y \
    ros-melodic-ros-core=1.4.1-0* \
&&  rm -rf /var/lib/apt/lists/*


# ROS Turtlesim
RUN apt-get update --fix-missing && \
    apt-get install -y --no-install-recommends \
    ros-melodic-turtlesim && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*


RUN apt-get update --fix-missing && \
    apt-get install -y --no-install-recommends \
    python-pip \
    python-catkin-tools && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN pip install -U \
    rosdep rosinstall_generator wstool rosinstall six vcstools

RUN rm -rf /root/.ros && \
    rm -rf /etc/ros/rosdep/sources.list.d/20-default.list
