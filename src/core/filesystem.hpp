#ifndef CORE_FILESYSTEM_HPP
#define CORE_FILESYSTEM_HPP

//check if the C++17 filesystem header exists
#if __has_include(<filesystem>)
  #include <filesystem>
  namespace fs=std::filesystem;
  
//if missing, check if C++14 experimental header exists
#elif __has_include(<experimental/filesystem>)
  #include <experimental/filesystem>
  namespace fs=std::experimental::filesystem;
  
#else
  #error "Fatal: Neither standard C++17 <filesystem> nor <experimental/filesystem> is supported by this compiler."
#endif

#endif