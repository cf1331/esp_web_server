#include <cjf/middleware/log_requests.h>
#include <cjf/web_server.h>

#include <esp_log.h>

namespace cjf
{

  log_requests::log_requests(const log_requests_config_t &config)
      : middleware_t({name, log_requests_handler, this}), _config(config)
  {
  }

  esp_err_t log_requests::log_requests_handler(httpd_req_t *req, middleware_next_t next)
  {
    auto self = reinterpret_cast<log_requests *>(req->user_ctx);
    auto method = static_cast<httpd_method_t>(req->method);
    ESP_LOGI(self->_config.tag, "Entry");
    TickType_t start = xTaskGetTickCount();
    ESP_LOGI(self->_config.tag, "%s \"%s\"", http_method_str(method), req->uri);
    esp_err_t ret = next();
    uint32_t duration_ms = pdTICKS_TO_MS(xTaskGetTickCount() - start);
    ESP_LOGI(self->_config.tag, "Response took %lums", duration_ms);
    return ret;
  }

} // namespace cjf
