# 设置项目名称  
cmake_minimum_required(VERSION 3.10)
set( MyProject "tinyfix")
set( MyLib "tinyfix")
set( MyExecutable "fix_test")
project(${MyProject})  
  
# 添加编译需要的头文件目录（如果有的话）  
# include_directories(include/)  
  
# 添加源文件所在的目录  
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
list(REMOVE_ITEM SRC_FILES "src/main.cpp")
  
# 添加可执行文件
add_library(${MyLib} SHARED ${SRC_FILES})
add_executable(${MyExecutable} "src/main.cpp")
  
# 如果需要链接其他库，可以添加如下命令  
# target_link_libraries(MyExecutable YourLibrary)  
  
# 设置安装目标（可选）  
# install(TARGETS MyExecutable DESTINATION bin)