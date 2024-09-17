#ifndef F2AB20FE_7446_4607_94B0_885ED8481F43
#define F2AB20FE_7446_4607_94B0_885ED8481F43

#include <esp_http_server.h>
#include <string>

namespace cjf
{

extern const char *MIME_UNKNOWN;

void mime_register(const char *mime, const char *file_extension);
void mime_register(const std::string &mime, const std::string &file_extension);

const char *mime_from_path(const std::string path);
esp_err_t set_content_type_from_path(httpd_req_t *req, const std::string path);

} // namespace cjf

#endif /* F2AB20FE_7446_4607_94B0_885ED8481F43 */
