#pragma once

namespace jv {

inline namespace v1 {
  #ifdef __PRETTY_FUNCTION__
  # define __ASSERT_FUNCTION __PRETTY_FUNCTION__
  #elif __func__
  # define __ASSERT_FUNCTION __func__
  #elif defined(__GNUC__)
  # define __ASSERT_FUNCTION __func__
  #else
  # define __ASSERT_FUNCTION ((__const char *) "")
  #endif

  extern void assert_faild(const char* assertion, const char* file_name, int line, const char* func_name);
  #define _TO_STRING(x) #x
  #define JV_ASSERT(expr) \
    do{ (expr) ? static_cast<void>(0) : jv::assert_faild(_TO_STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION); } while(false)

}// v1
}