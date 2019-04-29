# freertos-inside-ros
FreeRTOS inside GNU Linux ROS

## Create your own docker
docker build --rm -t ros-melodic .

## Compile freertos hello library
cd freertos_ros/ROS
make clean
make

## Create your ROS node
(edit your CMakeList.txt and package.xml)
cd ros_workspace/src
catkin_init_workspace
cd ..
catkin_make

