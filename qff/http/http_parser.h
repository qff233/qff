#ifndef __QFF_HTTP_PARSER_H__
#define __QFF_HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace qff::http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;

    HttpRequestParser();

    size_t execute(char* data, size_t len);
    int is_finished();
    int has_error(); 

    HttpRequest::ptr get_data() const { return m_data;}
    void set_error(int v) { m_error = v;}
    uint64_t get_content_length();
    const http_parser& get_parser() const { return m_parser;}
public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    /// 1000: invalid method
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;

    HttpResponseParser();

    size_t execute(char* data, size_t len, bool chunck);
    int is_finished();
    int has_error(); 

    HttpResponse::ptr get_data() const { return m_data;}

    void set_error(int v) { m_error = v;}
    uint64_t get_content_length();
    const httpclient_parser& get_parser() const { return m_parser;}
public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    /// 错误码
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

}

#endif