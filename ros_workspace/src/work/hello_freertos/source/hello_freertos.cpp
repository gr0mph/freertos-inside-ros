/*
* GitHub from gr0mph
* https://github.com/gr0mph
*	portable --> FreeRTOS inside GNU Linux ROS
 */
// Include the ROS C++ APIs
#include <ros/ros.h>
#include <port.h>
#include <pthread.h>

unsigned char g_sync;

void *startRTOS(void *arguments){
  //g_sync = 1;
  vStartFreeRTOS();
}

void *tickRTOS(void *arguments){

  //ros::Rate loop_rate(200);
  ros::Rate loop_rate(20);

  while ( ros::ok() )
  {

    vPortSysTickHandler(0);

    //ROS_INFO_STREAM("Tick!");

    ros::spinOnce();

    loop_rate.sleep();

  }

}

// Standard C++ entry point
int main(int argc, char** argv) {
  // Announce this program to the ROS master as a "node" called "hello_world_node"
  ros::init(argc, argv, "hello_world_node");
  // Start the node resource managers (communication, time, etc)
  ros::start();
  // Broadcast a simple log message
  ROS_INFO_STREAM("Hello, world!");

  pthread_t thread_RTOS;

  g_sync = 0;

  //pthread_create(&thread_RTOS, NULL, startRTOS, NULL);
  pthread_create(&thread_RTOS, NULL, tickRTOS, NULL);

  vStartFreeRTOS();

  // Process ROS callbacks until receiving a SIGINT (ctrl-c)
  ros::spin();
  // Stop the node's resources
  ros::shutdown();
  // Exit tranquilly
  return 0;
}
