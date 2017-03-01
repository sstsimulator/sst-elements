//---------------------------------------------------------
// Copyright 2015 Ontario Institute for Cancer Research
// Written by Matei David (matei@cs.toronto.edu)
//---------------------------------------------------------
//
// Reference:
// http://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
//
// Copied from: https://github.com/mateidavid/zstr.git on 1 March 2017
//      branch: master 
//      commit: 74c5a7cd27291b57fd73bc570588961efa7ba18e
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

#include "c_zstr.hpp"

using namespace zstr;

Exception::Exception(z_stream * zstrm_p, int ret)
  : _msg("zlib: ")
{
  switch (ret)
  {
  case Z_STREAM_ERROR:
    _msg += "Z_STREAM_ERROR: ";
    break;
  case Z_DATA_ERROR:
    _msg += "Z_DATA_ERROR: ";
    break;
  case Z_MEM_ERROR:
    _msg += "Z_MEM_ERROR: ";
    break;
  case Z_VERSION_ERROR:
    _msg += "Z_VERSION_ERROR: ";
    break;
  case Z_BUF_ERROR:
    _msg += "Z_BUF_ERROR: ";
    break;
  default:
    std::ostringstream oss;
    oss << ret;
    _msg += "[" + oss.str() + "]: ";
    break;
  }
  _msg += zstrm_p->msg;
}

const char * zstr::Exception::what() const noexcept { return _msg.c_str(); }



