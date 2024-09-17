#include <cjf/mime.h>

#include <esp_err.h>
#include <esp_http_server.h>
#include <map>
#include <string>

namespace cjf
{

  static std::map<std::string, const char *> MIME_BY_EXT = {
      {".htm", "text/html"},
      {".html", "text/html"},
      {".css", "text/css"},
      {".jpg", "image/jpeg"},
      {".jpeg", "image/jpeg"},
      {".js", "application/javascript"},
      {".json", "application/json"},
      {".ico", "image/x-icon"},
      {".svg", "image/svg+xml"}};

  void mime_register(const char *mime, const char *file_extension)
  {
    MIME_BY_EXT[file_extension] = mime;
  }

  void mime_register(const std::string &mime, const std::string &file_extension)
  {
    MIME_BY_EXT[file_extension] = mime.c_str();
  }

  const char *mime_from_path(const std::string path)
  {
    auto ext = path.substr(path.find_last_of("."));
    auto match = MIME_BY_EXT.find(ext);
    if (match != MIME_BY_EXT.end())
    {
      return match->second;
    }
    return nullptr;
  }

  esp_err_t set_content_type_from_path(httpd_req_t *req, const std::string path)
  {
    auto mime = mime_from_path(path);
    if (!mime)
    {
      mime = MIME_UNKNOWN;
    }
    return httpd_resp_set_type(req, mime);
  }

  const char *MIME_UNKNOWN = "application/octet-stream";

} // namespace cjf
