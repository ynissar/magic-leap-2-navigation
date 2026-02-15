// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
// Copyright (c) 2021 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Software License Agreement,
// located here: https://www.magicleap.com/software-license-agreement-ml2
// Terms and conditions applicable to third-party materials accompanying
// this distribution may also be found in the top-level NOTICE file
// appearing herein.
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%

#include <app_framework/web_request.h>

namespace ml {
namespace app_framework {

WebRequest::WebRequest(struct android_app *app) : read_size_(0x10000), connect_timeout_(5000) {
  // App Sim code goes here
  (void)app;
}

WebRequest::~WebRequest() {}

void WebRequest::SetReadSize(size_t read_size) {
  read_size_ = read_size;
}

void WebRequest::SetConnectionTimeout(size_t connect_timeout) {
  connect_timeout_ = connect_timeout;
}

WebResponse WebRequest::request(const std::string &url, HTTP_METHOD method, const HTTPHeaderList &request_headers,
                                std::function<void(const char *data, int size, const void *context)> data_available,
                                const WebRequestPayloadBuffer *request_payload) {
  // App Sim code goes here
  (void)url;
  (void)method;
  (void)request_headers;
  (void)data_available;
  (void)request_payload;

  return WebResponse();
}

}  // namespace app_framework
}  // namespace ml
