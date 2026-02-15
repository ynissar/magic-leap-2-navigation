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

#include <app_framework/system_helper_common.h>
#include <app_framework/web_request.h>

namespace ml {
namespace app_framework {

//  trim from start (in place)
static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

//  trim from end (in place)
static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) {
                         return !std::isspace(ch);
                       })
              .base(),
          s.end());
}

//  trim from both ends (in place)
static inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

static inline const char *http_method_to_string(enum HTTP_METHOD method) {
  static const char *strings[] = {"GET", "POST", "PUT", "DELETE"};
  return strings[method];
}

std::string ToString(ScopedJNIAttach &env, const jstring jstr) {
  const char *pack_utf_chars = env->GetStringUTFChars(jstr, nullptr);
  std::string cstr(pack_utf_chars);
  env->ReleaseStringUTFChars(jstr, pack_utf_chars);
  return cstr;
}

WebRequest::WebRequest(struct android_app *app) :
    vm_(app->activity->vm),
    url_cls_(0),
    url_ctor_mid_(0),
    url_open_connection_mid_(0),
    http_url_connection_cls_(0),
    set_request_method_mid_(0),
    set_request_property_mid_(0),
    connect_mid_(0),
    set_connect_timout_mid_(0),
    set_read_timout_mid_(0),
    set_do_output_mid_(0),
    get_response_code_mid_(0),
    get_input_stream_mid_(0),
    get_error_stream_mid_(0),
    get_output_stream_mid_(0),
    input_stream_cls_(0),
    input_stream_read_mid_(0),
    input_stream_close_mid_(0),
    output_stream_cls_(0),
    output_stream_write_mid_(0),
    output_stream_close_mid_(0),
    get_header_field_mid_(0),
    get_header_field_key_mid_(0),
    read_size_(0x10000),
    connect_timeout_(5000) {
  ScopedJNIAttach jni_scope(vm_);

  url_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(jni_scope->FindClass("java/net/URL")));
  url_ctor_mid_ = jni_scope->GetMethodID(url_cls_, "<init>", "(Ljava/lang/String;)V");
  url_open_connection_mid_ = jni_scope->GetMethodID(url_cls_, "openConnection", "()Ljava/net/URLConnection;");
  http_url_connection_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(jni_scope->FindClass("javax/net/ssl/"
                                                                                                   "HttpsURLConnectio"
                                                                                                   "n")));
  set_request_method_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "setRequestMethod",
                                                   "(Ljava/lang/String;)V");
  set_request_property_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "setRequestProperty",
                                                     "(Ljava/lang/String;Ljava/lang/String;)V");
  connect_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "connect", "()V");
  set_connect_timout_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "setConnectTimeout", "(I)V");
  set_read_timout_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "setReadTimeout", "(I)V");
  set_do_output_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "setDoOutput", "(Z)V");
  get_response_code_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "getResponseCode", "()I");
  get_input_stream_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "getInputStream", "()Ljava/io/InputStream;");
  get_error_stream_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "getErrorStream", "()Ljava/io/InputStream;");
  get_output_stream_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "getOutputStream",
                                                  "()Ljava/io/OutputStream;");
  input_stream_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(jni_scope->FindClass("java/io/InputStream")));
  input_stream_read_mid_ = jni_scope->GetMethodID(input_stream_cls_, "read", "([BII)I");
  input_stream_close_mid_ = jni_scope->GetMethodID(input_stream_cls_, "close", "()V");
  output_stream_cls_ = reinterpret_cast<jclass>(jni_scope->NewGlobalRef(jni_scope->FindClass("java/io/OutputStream")));
  output_stream_write_mid_ = jni_scope->GetMethodID(output_stream_cls_, "write", "([BII)V");
  output_stream_close_mid_ = jni_scope->GetMethodID(output_stream_cls_, "close", "()V");
  get_header_field_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "getHeaderField", "(I)Ljava/lang/String;");
  get_header_field_key_mid_ = jni_scope->GetMethodID(http_url_connection_cls_, "getHeaderFieldKey",
                                                     "(I)Ljava/lang/String;");
}

WebRequest::~WebRequest() {
  ScopedJNIAttach jni_scope(vm_);

  if (url_cls_) jni_scope->DeleteGlobalRef(url_cls_);
  if (http_url_connection_cls_) jni_scope->DeleteGlobalRef(http_url_connection_cls_);
  if (input_stream_cls_) jni_scope->DeleteGlobalRef(input_stream_cls_);
  if (output_stream_cls_) jni_scope->DeleteGlobalRef(output_stream_cls_);
}

void WebRequest::SetReadSize(size_t read_size) {
  read_size_ = read_size;
}

void WebRequest::SetConnectionTimeout(size_t connect_timeout) {
  connect_timeout_ = connect_timeout;
}

WebResponse WebRequest::request(const std::string &url, HTTP_METHOD method, const HTTPHeaderList &request_headers,
                                std::function<void(const char *data, int size, const void *context)> data_available,
                                const WebRequestPayloadBuffer *request_payload) {
  ScopedJNIAttach jni_scope(vm_);

  //  create url object
  jstring url_str = jni_scope->NewStringUTF(url.c_str());
  jobject url_obj = NEW_OBJECT(false, url_cls_, url_ctor_mid_, url_str);
  jni_scope->DeleteLocalRef(url_str);

  //  open connection
  jobject connection = CALL_OBJECT_METHOD(false, url_obj, url_open_connection_mid_);

  //  set http method (GET, POST, etc)
  jstring method_str = jni_scope->NewStringUTF(http_method_to_string(method));
  CALL_VOID_METHOD(false, connection, set_request_method_mid_, method_str);
  jni_scope->DeleteLocalRef(method_str);

  //  set headers
  for (const auto &header : request_headers) {
    jstring key = jni_scope->NewStringUTF(header.first.c_str());
    jstring val = jni_scope->NewStringUTF(header.second.c_str());

    CALL_VOID_METHOD(false, connection, set_request_property_mid_, key, val);

    jni_scope->DeleteLocalRef(key);
    jni_scope->DeleteLocalRef(val);
  }

  //  set timeouts
  CALL_VOID_METHOD(false, connection, set_connect_timout_mid_, (jint)connect_timeout_);
  CALL_VOID_METHOD(false, connection, set_read_timout_mid_, (jint)connect_timeout_);

  //  upload request body when provided
  if (request_payload != nullptr) {
    CALL_VOID_METHOD(false, connection, set_do_output_mid_, (jboolean) true);
    jobject output_stream = CALL_OBJECT_METHOD(false, connection, get_output_stream_mid_);
    jbyteArray buffer = jni_scope->NewByteArray(request_payload->buffer_length);
    if (buffer != nullptr) {
      jni_scope->SetByteArrayRegion(buffer, 0, request_payload->buffer_length, (jbyte *)request_payload->buffer_ptr);
      CALL_VOID_METHOD(false, output_stream, output_stream_write_mid_, buffer, (jint)0,
                       (jint)request_payload->buffer_length);
      CALL_VOID_METHOD(false, output_stream, output_stream_close_mid_);
    } else {
      ALOGE("Failed to allocate JNI byte array of size: %lu for request payload.", request_payload->buffer_length);
    }
  }

  WebResponse response;

  //  connect (return in case of exception):
  jni_scope->CallVoidMethod(connection, connect_mid_);
  if (jni_scope->ExceptionCheck()) {
    jni_scope->ExceptionDescribe();
    jni_scope->ExceptionClear();
    ALOGI("CallVoidMethod() exception, see logcat for more details.");
    return response;
  }

  //  get response code
  response.result = CALL_INT_METHOD(false, connection, get_response_code_mid_);

  //  read data if data_available callback is set
  if (data_available) {
    auto getStreamMethodId = response.result >= 400 && response.result <= 599 ? get_error_stream_mid_ :
                                                                                get_input_stream_mid_;
    jobject input_stream = CALL_OBJECT_METHOD(false, connection, getStreamMethodId);
    // check if response has a payload
    if (input_stream != nullptr) {
      jbyteArray buffer = jni_scope->NewByteArray(read_size_);
      jint read = 0;
      jint buffer_len = (jint)jni_scope->GetArrayLength(buffer);

      do {
        //  read data
        read = CALL_INT_METHOD(false, input_stream, input_stream_read_mid_, buffer, 0, buffer_len);
        if (read > 0) {
          jbyte *tmp = jni_scope->GetByteArrayElements(buffer, NULL);
          data_available((const char *)tmp, read, (void *)this);
          jni_scope->ReleaseByteArrayElements(buffer, tmp, 0);
        }
      } while (read >= 0);

      //  indicate we are done reading
      data_available(nullptr, 0, (void *)this);

      //  close input stream
      CALL_VOID_METHOD(false, input_stream, input_stream_close_mid_);
    }
  }

  //  get response headers
  for (jint i = 0;; i++) {
    //  header field key
    jstring key_str = (jstring)CALL_OBJECT_METHOD(false, connection, get_header_field_key_mid_, i);
    if (!key_str) break;

    //  header field value
    jstring val_str = (jstring)CALL_OBJECT_METHOD(false, connection, get_header_field_mid_, i);
    if (!val_str) break;

    std::string key = ToString(jni_scope, key_str);
    std::string val = ToString(jni_scope, val_str);

    trim(key);
    trim(val);

    response.headers.push_back(std::pair<std::string, std::string>(key, val));
  }

  return response;
}

}  // namespace app_framework
}  // namespace ml
