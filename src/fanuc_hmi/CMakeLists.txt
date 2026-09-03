cmake_minimum_required(VERSION 3.16)
project(fanuc_hmi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(control_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(fanuc_hmi
  src/main.cpp
  src/MainWindow.cpp
  src/FanucRosBridge.cpp
  include/fanuc_hmi/MainWindow.h
  include/fanuc_hmi/FanucRosBridge.h
)

target_include_directories(fanuc_hmi PRIVATE include)

# Keep the plain target_link_libraries signature because ament_target_dependencies
# also links dependencies using the plain signature on ROS 2 Jazzy.
target_link_libraries(fanuc_hmi
  Qt6::Core
  Qt6::Widgets
)

ament_target_dependencies(fanuc_hmi
  rclcpp
  rclcpp_action
  sensor_msgs
  control_msgs
  trajectory_msgs
)

install(TARGETS fanuc_hmi
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
