# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/Espressif/frameworks/esp-idf-v4.4.4/components/bootloader/subproject"
  "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/GraduateThesis-Project/build/bootloader"
  "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/GraduateThesis-Project/build/bootloader-prefix"
  "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/GraduateThesis-Project/build/bootloader-prefix/tmp"
  "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/GraduateThesis-Project/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/GraduateThesis-Project/build/bootloader-prefix/src"
  "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/GraduateThesis-Project/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/lamqu/Documents/Tri/Laptrinh/Esp/GraduateThesis-Project/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
