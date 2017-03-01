// Copied from: https://github.com/mateidavid/zstr.git on 1 March 2017
//      branch: master 
//      commit: 74c5a7cd27291b57fd73bc570588961efa7ba18e
//
// Used by c_zstr.hpp
//
// LICENSE:
// The MIT License (MIT)
//
// Copyright (c) 2015 Matei David, Ontario Institute for Cancer Research
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE. 

#include "c_strict_fstream.hpp"

using namespace strict_fstream;

std::string strerror()
{
  std::string buff(80, '\0');
#ifdef _WIN32
  if (strerror_s(&buff[0], buff.size(), errno) != 0)
  {
    buff = "Unknown error";
  }
#elif (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
  // XSI-compliant strerror_r()
  if (strerror_r(errno, &buff[0], buff.size()) != 0)
  {
    buff = "Unknown error";
  }
#else
  // GNU-specific strerror_r()
  auto p = strerror_r(errno, &buff[0], buff.size());
  std::string tmp(p, std::strlen(p));
  std::swap(buff, tmp);
#endif
  buff.resize(buff.find('\0'));
  return buff;
}

