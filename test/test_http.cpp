#include "log.h"
#include "http/http_parser.h"

const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Host: qff233.com\r\n"
                                "Content-Length: 10\r\n\r\n"
                                "1234567890";

void test_request() {
    qff::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t s = parser.execute(&tmp[0], tmp.size());
    if(s < 0)
        QFF_LOG_ERROR(QFF_LOG_ROOT) << "execute rt=" << s
            << "has_error=" << parser.has_error()
            << " is_finished=" << parser.is_finished()
            << " total=" << tmp.size()
            << " content_length=" << parser.get_content_length();
    tmp.resize(tmp.size() - s);
    QFF_LOG_INFO(QFF_LOG_ROOT) << parser.get_data()->to_string();
    QFF_LOG_INFO(QFF_LOG_ROOT) << tmp;
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";

void test_response() {
    qff::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size(), true);
    QFF_LOG_INFO(QFF_LOG_ROOT) << "execute rt=" << s
        << " has_error=" << parser.has_error()
        << " is_finished=" << parser.is_finished()
        << " total=" << tmp.size()
        << " content_length=" << parser.get_content_length()
        << " tmp[s]=" << tmp[s];

    tmp.resize(tmp.size() - s);

    QFF_LOG_INFO(QFF_LOG_ROOT) << parser.get_data()->to_string();
    QFF_LOG_INFO(QFF_LOG_ROOT) << tmp;
}

int main(int argc, char** argv) {
    qff::LoggerMgr::New();
    test_request();
    //QFF_LOG_INFO(QFF_LOG_ROOT) << "--------------";
    //test_response();
    return 0;
}