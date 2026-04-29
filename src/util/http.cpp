#include "util/http.h"

#include <curl/curl.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <sstream>

namespace bagu::util {

namespace {

// libcurl write callback（一次性收完）
size_t write_to_string(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// 流式回调：按行切分（SSE 用）
struct StreamCtx {
    std::function<void(const std::string&)>* on_line;
    std::string buffer;
};

size_t write_stream_line(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ctx = static_cast<StreamCtx*>(userdata);
    ctx->buffer.append(static_cast<char*>(ptr), size * nmemb);
    size_t pos;
    while ((pos = ctx->buffer.find('\n')) != std::string::npos) {
        std::string line = ctx->buffer.substr(0, pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (ctx->on_line && *ctx->on_line) (*ctx->on_line)(line);
        ctx->buffer.erase(0, pos + 1);
    }
    return size * nmemb;
}

curl_slist* make_header_list(const std::vector<std::string>& headers) {
    curl_slist* list = nullptr;
    for (const auto& h : headers) {
        list = curl_slist_append(list, h.c_str());
    }
    return list;
}

}  // namespace

Result<HttpResponse> http_post_json(const std::string& url,
                                    const std::string& json_body,
                                    const std::vector<std::string>& headers,
                                    int timeout_seconds) {
    CURL* curl = curl_easy_init();
    if (!curl) return make_err<HttpResponse>(E::kNetworkError, "curl init failed");

    HttpResponse resp;
    auto* hdr_list = make_header_list(headers);
    if (hdr_list == nullptr ||
        std::find_if(headers.begin(), headers.end(), [](const std::string& h){
            return h.find("Content-Type") != std::string::npos;
        }) == headers.end()) {
        hdr_list = curl_slist_append(hdr_list, "Content-Type: application/json");
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr_list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        std::string err = curl_easy_strerror(rc);
        curl_slist_free_all(hdr_list);
        curl_easy_cleanup(curl);
        return make_err<HttpResponse>(E::kNetworkError, "curl perform failed",
            "url=" + url + " err=" + err);
    }

    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    resp.status_code = static_cast<int>(code);

    curl_slist_free_all(hdr_list);
    curl_easy_cleanup(curl);
    return resp;
}

Result<int> http_post_json_stream(const std::string& url,
                                  const std::string& json_body,
                                  const std::vector<std::string>& headers,
                                  std::function<void(const std::string&)> on_line,
                                  int timeout_seconds) {
    CURL* curl = curl_easy_init();
    if (!curl) return make_err<int>(E::kNetworkError, "curl init failed");

    StreamCtx ctx{&on_line, {}};
    auto* hdr_list = make_header_list(headers);
    bool has_ct = false;
    for (auto& h : headers) {
        if (h.find("Content-Type") != std::string::npos) { has_ct = true; break; }
    }
    if (!has_ct) hdr_list = curl_slist_append(hdr_list, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr_list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_stream_line);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        std::string err = curl_easy_strerror(rc);
        curl_slist_free_all(hdr_list);
        curl_easy_cleanup(curl);
        return make_err<int>(E::kNetworkError, "curl stream failed",
            "url=" + url + " err=" + err);
    }

    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    // flush 残留 buffer
    if (!ctx.buffer.empty() && on_line) on_line(ctx.buffer);

    curl_slist_free_all(hdr_list);
    curl_easy_cleanup(curl);
    return static_cast<int>(code);
}

}  // namespace bagu::util
