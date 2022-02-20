#include "http_parser.h"

#include "http.h"
#include "../log.h"

namespace qff::http {

//TODO
static uint64_t g_http_request_buffer_size = 4 * 1024;
static uint64_t g_http_request_max_body_size = 64 * 1024 * 1024;
static uint64_t g_http_response_buffer_size = 4 * 1024;
static uint64_t g_http_response_max_body_size = 64 * 1024 * 1024;

static uint64_t s_http_request_buffer_size = g_http_request_buffer_size;
static uint64_t s_http_request_max_body_size = g_http_request_max_body_size;
static uint64_t s_http_response_buffer_size = g_http_response_buffer_size;
static uint64_t s_http_response_max_body_size = g_http_response_max_body_size;

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize() {
    return s_http_response_max_body_size;
}

static void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = StringToHttpMethod(at);

    if(m == HttpMethod::INVALID_METHOD) {
        QFF_LOG_WARN(QFF_LOG_SYSTEM) << "invalid http request method: "
            << std::string(at, length);
        parser->set_error(1000);
        return;
    }
    parser->get_data()->method = m;
}

static void on_request_uri(void *data, const char *at, size_t length) {
}

static void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->get_data()->fragment = std::move(std::string(at, length));
}

static void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->get_data()->path = std::move(std::string(at, length));
}

static void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->get_data()->query = std::move(std::string(at, length));
}

static void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        QFF_LOG_WARN(QFF_LOG_SYSTEM) << "invalid http request version: "
            << std::string(at, length);
        parser->set_error(1001);
        return;
    }
    parser->get_data()->version = v;
}

static void on_request_header_done(void *data, const char *at, size_t length) {
}

static void on_request_http_field(void *data, const char *field, size_t flen
                           ,const char *value, size_t vlen) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if(flen == 0) {
        QFF_LOG_WARN(QFF_LOG_SYSTEM) << "invalid http request field length == 0";
        parser->set_error(1002);
        return;
    }
    parser->get_data()->set_header(std::string(field, flen)
                                ,std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser()
    :m_error(0) {
    m_data = std::make_shared<HttpRequest>();
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}

size_t HttpRequestParser::execute(char* data, size_t len) {
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpRequestParser::is_finished() {
    return http_parser_finish(&m_parser);
}

int HttpRequestParser::has_error() {
    return m_error || http_parser_has_error(&m_parser);
}

uint64_t HttpRequestParser::get_content_length() {
    return m_data->get_header_as<uint64_t>("content-length", 0);
}

static void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->get_data()->reason = std::move(std::string(at, length));
}

static void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at));
    parser->get_data()->status = status;
}

static void on_response_chunk(void *data, const char *at, size_t length) {
}

static void on_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        QFF_LOG_WARN(QFF_LOG_SYSTEM) << "invalid http response version: "
            << std::string(at, length);
        parser->set_error(1001);
        return;
    }

    parser->get_data()->version = v;
}

static void on_response_header_done(void *data, const char *at, size_t length) {
}

static void on_response_last_chunk(void *data, const char *at, size_t length) {
}

static void on_response_http_field(void *data, const char *field, size_t flen
                           ,const char *value, size_t vlen) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0) {
        QFF_LOG_WARN(QFF_LOG_SYSTEM) << "invalid http response field length == 0";
        parser->set_error(1002);
        return;
    }
    parser->get_data()->set_header(std::string(field, flen)
                                ,std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser()
    :m_error(0) {
    m_data  = std::make_shared<HttpResponse>();
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}


size_t HttpResponseParser::execute(char* data, size_t len, bool chunck) {
    if(chunck) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpResponseParser::is_finished() {
    return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::has_error() {
    return m_error || httpclient_parser_has_error(&m_parser);
}

uint64_t HttpResponseParser::get_content_length() {
    return m_data->get_header_as<uint64_t>("content-length", 0);
}


}