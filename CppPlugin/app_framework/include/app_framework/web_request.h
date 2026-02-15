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
#pragma once

#include <android_native_app_glue.h>
#include <app_framework/logging.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef ML_LUMIN
#include <jni.h>
#endif

namespace ml {
namespace app_framework {

typedef std::vector<std::pair<std::string, std::string>> HTTPHeaderList;

enum HTTP_METHOD { HTTP_GET = 0, HTTP_POST, HTTP_PUT, HTTP_DELETE };

struct WebResponse {
  HTTPHeaderList headers;
  int result = -1;
};

struct WebRequestPayloadBuffer {
  int8_t *buffer_ptr;
  size_t buffer_length;
};

class WebRequest {
private:
#if ML_LUMIN
  JavaVM *vm_;
  jclass url_cls_;
  jmethodID url_ctor_mid_;
  jmethodID url_open_connection_mid_;
  jclass http_url_connection_cls_;
  jmethodID set_request_method_mid_;
  jmethodID set_request_property_mid_;
  jmethodID connect_mid_;
  jmethodID set_connect_timout_mid_;
  jmethodID set_read_timout_mid_;
  jmethodID set_do_output_mid_;
  jmethodID get_response_code_mid_;
  jmethodID get_input_stream_mid_;
  jmethodID get_error_stream_mid_;
  jmethodID get_output_stream_mid_;
  jclass input_stream_cls_;
  jmethodID input_stream_read_mid_;
  jmethodID input_stream_close_mid_;
  jclass output_stream_cls_;
  jmethodID output_stream_write_mid_;
  jmethodID output_stream_close_mid_;
  jmethodID get_header_field_mid_;
  jmethodID get_header_field_key_mid_;
#endif

  size_t read_size_;
  size_t connect_timeout_;

public:
  WebRequest(struct android_app *app);
  ~WebRequest();

  void SetReadSize(size_t read_size);
  void SetConnectionTimeout(size_t connect_timeout);

  WebResponse request(const std::string &url, enum HTTP_METHOD method = HTTP_GET,
                      const HTTPHeaderList &request_headers = {},
                      std::function<void(const char *data, int size, const void *context)> callback = nullptr,
                      const WebRequestPayloadBuffer *request_payload = nullptr);
};

}  // namespace app_framework
}  // namespace ml