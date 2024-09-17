#include <cjf/middleware/not_found.h>
#include <cjf/web_server.h>
#include <esp_http_server.h>

namespace cjf
{

  not_found::not_found()
      : middleware_t({name, not_found_handler, this})
  {
  }

  esp_err_t not_found::not_found_handler(httpd_req_t *req, middleware_next_t next)
  {
    httpd_resp_send_404(req);
    return ESP_OK;
  }

} // namespace cjf
