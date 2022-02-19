#include "http.h"

#include "../utils.h"

namespace qff::http {
    
HttpMethod StringToHttpMethod(std::string_view str) {
#define XX(num, name, string)                   \
    if(::strcmp(#string, str.data()) == 0) {    \
        return HttpMethod::name;                \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char* s_method_string[] = {
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

std::string_view HttpMethodToString(const HttpMethod method) {
    uint32_t index = (uint32_t)method;
    if(index >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
        return "<unknown>";
    }
    return s_method_string[index];
}

std::string_view HttpStatusToString(const HttpStatus method) {
    switch(method) {
#define XX(code, name, msg) \
        case HttpStatus::name: \
            return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}

bool CaseInsensitiveLess::operator() (std::string_view lhs, std::string_view rhs) const {
    return ::strcasecmp(lhs.data(), rhs.data()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool keep_alive)
    :method(HttpMethod::GET)
    ,version(version)
    ,keep_alive(keep_alive)
    ,websocket(false)
    ,path("/") {
}

std::shared_ptr<HttpResponse> HttpRequest::create_response() {
    return std::make_shared<HttpResponse>(this->version, this->keep_alive);
}

std::string HttpRequest::get_header(std::string_view key, std::string_view def) const {
    auto it = m_headers.find(key.data());
    return it == m_headers.end() ? def.data() : it->second;
}

std::string HttpRequest::get_param(std::string_view key, std::string_view def) {
    this->init_query_param();
    this->init_body_param();
    auto it = m_params.find(key.data());
    return it == m_params.end() ? def.data() : it->second;
}

std::string HttpRequest::get_cookie(std::string_view key, std::string_view def) {
    this->init_cookies();
    auto it = m_cookies.find(key.data());
    return it == m_cookies.end() ? def.data() : it->second;
}

void HttpRequest::set_header(std::string_view key, std::string_view val) {
    m_headers[key.data()] = val;
}

void HttpRequest::set_param(std::string_view key, std::string_view val) {
    m_params[key.data()] = val;
}

void HttpRequest::set_cookie(std::string_view key, std::string_view val) {
    m_cookies[key.data()] = val;
}

void HttpRequest::del_header(std::string_view key) {
    m_headers.erase(key.data());
}

void HttpRequest::del_param(std::string_view key) {
    m_params.erase(key.data());
}

void HttpRequest::del_cookie(std::string_view key) {
    m_cookies.erase(key.data());
}

bool HttpRequest::has_header(std::string_view key, std::string* val) {
    auto it = m_headers.find(key.data());
    if(it == m_headers.end())
        return false;
    if(val)
        *val = it->second;
    return true;
}

bool HttpRequest::has_param(std::string_view key, std::string* val) {
    this->init_query_param();
    this->init_body_param();
    auto it = m_params.find(key.data());
    if(it == m_params.end())
        return false;
    if(val)
        *val = it->second;
    return true;
}

bool HttpRequest::has_cookie(std::string_view key, std::string* val) {
    this->init_cookies();
    auto it = m_cookies.find(key.data());
    if(it == m_cookies.end())
        return false;
    if(val)
        *val = it->second;
    return true;
}

std::ostream& HttpRequest::dump(std::ostream& os) const {
    os << HttpMethodToString(this->method) << " "
       << this->path
       << (this->query.empty() ? "" : "?")
       << this->query
       << (this->fragment.empty() ? "" : "#")
       << this->fragment
       << " HTTP/"
       << ((uint32_t)(this->version >> 4))
       << "."
       << ((uint32_t)(this->version & 0x0F))
       << "\r\n";
    if(!this->websocket) {
        os << "connection: " << (this->keep_alive ? "keep-alive" : "close") << "\r\n";
    }
    for(auto& i : m_headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0 && !this->websocket) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }

    if(!this->body.empty()) {
        os << "content-length: " << this->body.size() << "\r\n\r\n"
           << this->body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

void HttpRequest::init() {
    std::string conn = get_header("connection");
    if(!conn.empty()) {
        if(strcasecmp(conn.c_str(), "keep-alive") == 0)
            this->keep_alive = true;
        else
            this->keep_alive = false;
    }
}

void HttpRequest::init_param() {
    this->init_query_param();
    this->init_body_param();
    this->init_cookies();
}

void HttpRequest::init_query_param() {
    if(m_parser_param_flag & 0x1) {
        return;
    }
#define PARSE_PARAM(str, m, flag, trim) \
    size_t pos = 0; \
    do { \
        size_t last = pos; \
        pos = str.find('=', pos); \
        if(pos == std::string::npos) \
            break; \
        size_t key = pos; \
        pos = str.find(flag, pos); \
        m.insert(std::make_pair(trim(str.substr(last, key - last)), \
                    qff::StringUtils::UrlDecode(str.substr(key + 1, pos - key - 1)))); \
        if(pos == std::string::npos) { \
            break; \
        } \
        ++pos; \
    } while(true);

    PARSE_PARAM(query, m_params, '&',);
    m_parser_param_flag |= 0x1;
}

void HttpRequest::init_body_param() {
    if(m_parser_param_flag & 0x2) {
        return;
    }
    std::string content_type = get_header("content-type");
    if(strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr) {
        m_parser_param_flag |= 0x2;
        return;
    }
    PARSE_PARAM(body, m_params, '&',);
    m_parser_param_flag |= 0x2;
}

void HttpRequest::init_cookies() {
    if(m_parser_param_flag & 0x4) {
        return;
    }
    std::string cookie = get_header("cookie");
    if(cookie.empty()) {
        m_parser_param_flag |= 0x4;
        return;
    }
    PARSE_PARAM(cookie, m_cookies, ';', qff::StringUtils::Trim);
    m_parser_param_flag |= 0x4;
}

HttpResponse::HttpResponse(uint8_t version, bool keep_alive)
    :status(HttpStatus::OK)
    ,version(version)
    ,keep_alive(keep_alive)
    ,websocket(false) {
}

std::string HttpResponse::get_header(std::string_view key, std::string_view def) const {
    auto it = m_headers.find(key.data());
    return it == m_headers.end() ? def.data() : it->second;
}

void HttpResponse::set_header(std::string_view key, std::string_view val) {
    m_headers[key.data()] = val;
}

void HttpResponse::del_header(std::string_view key) {
    m_headers.erase(key.data());
}

std::ostream& HttpResponse::dump(std::ostream& os) const {
    os << "HTTP/"
       << ((uint32_t)(this->version >> 4))
       << "."
       << ((uint32_t)(this->version & 0x0F))
       << " "
       << (uint32_t)this->status
       << " "
       << (this->reason.empty() ? HttpStatusToString(this->status) : this->reason)
       << "\r\n";

    for(auto& i : m_headers) {
        if(!this->websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    for(auto& i : m_cookies) {
        os << "Set-Cookie: " << i << "\r\n";
    }
    if(!this->websocket) {
        os << "connection: " << (this->keep_alive ? "keep-alive" : "close") << "\r\n";
    }
    if(!this->body.empty()) {
        os << "content-length: " << this->body.size() << "\r\n\r\n"
           << this->body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpResponse::to_string() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

void HttpResponse::set_redirect(std::string_view uri) {
    status = HttpStatus::FOUND;
    set_header("Location", uri);
}
void HttpResponse::set_cookie(std::string_view key, std::string_view val,
               time_t expired, std::string_view path,
               std::string_view domain, bool secure) {
    std::stringstream ss;
    ss << key << "=" << val;
    if(expired > 0) {
        ss << ";expires=" << qff::Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
    }
    if(!domain.empty()) {
        ss << ";domain=" << domain;
    }
    if(!path.empty()) {
        ss << ";path=" << path;
    }
    if(secure) {
        ss << ";secure";
    }
    m_cookies.push_back(ss.str());
}

} // namespace qff
