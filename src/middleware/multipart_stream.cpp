#include <cjf/middleware/multipart_stream.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_http_server.h>
#include <freertos/event_groups.h>
#include <string>

namespace cjf
{

  const char *MULTIPART_STREAM_MIDDLEWARE = "middleware:multipart_stream";

  const EventBits_t PART_READY = 0x01;
  const EventBits_t PART_SENT = 0x02;

  constexpr std::string part_boundary(const char *boundary)
  {
    return std::string("\r\n--") + boundary + "\r\n";
  }

  constexpr std::string part_content_type(const char *content_type)
  {
    return std::string("Content-Type: ") + content_type + "\r\n";
  }

  constexpr std::string stream_content_type(const char *boundary)
  {
    return std::string("multipart/x-mixed-replace;boundary=") + boundary;
  }

  using deleter = void (*)(void *ctx);

  struct pending_part
  {
    const char *data;
    size_t size;
    deleter destroy = nullptr;
    void *ctx = nullptr;
  };

  multipart_stream::multipart_stream(const multipart_stream_config_t& config)
    : multipart_stream(name, config)
  {
  }

  multipart_stream::multipart_stream(const char* name, const multipart_stream_config_t config)
      : middleware_t({name, multipart_stream::multipart_stream_handler, this}),
        _config(config),
        _part_boundary(part_boundary(config.boundary)),
        _part_content_type(part_content_type(config.part_content_type)),
        _stream_content_type(stream_content_type(config.boundary))
  {
    _event_group = xEventGroupCreate();
    if (!_event_group)
    {
      ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
    // This must be set so that the first frame can be sent
    xEventGroupSetBits(_event_group, PART_SENT);
  }

  multipart_stream::~multipart_stream()
  {
    if (_event_group)
    {
      vEventGroupDelete(_event_group);
    }
  }

  esp_err_t multipart_stream::write(const char *data, const size_t size, TickType_t ticks_to_wait)
  {
    // If a timeout occurs while writing a part, there may still be a transfer in
    // progress. We need to wait for the transfer to complete before writing the next part.
    TickType_t start = xTaskGetTickCount();
    if (xEventGroupWaitBits(_event_group, PART_SENT, pdTRUE, pdTRUE, ticks_to_wait) != PART_SENT)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_TIMEOUT, MULTIPART_STREAM_MIDDLEWARE, "Timeout waiting for previous part to be sent");
    }
    ticks_to_wait -= xTaskGetTickCount() - start;
    _part = reinterpret_cast<const char *>(data);
    _part_size = size;
    xEventGroupSetBits(_event_group, PART_READY);
    // Wait until the part is sent or a timeout occurs. Do not clear the PART_SENT bit here,
    // it is used above to check that the previous part has been sent.
    if (xEventGroupWaitBits(_event_group, PART_SENT, pdFALSE, pdTRUE, ticks_to_wait) != PART_SENT)
    {
      ESP_RETURN_ON_ERROR(ESP_ERR_TIMEOUT, MULTIPART_STREAM_MIDDLEWARE, "Timeout waiting for part to be sent");
    }
    return ESP_OK;
  }

  esp_err_t multipart_stream::write(const uint8_t *data, const size_t size, TickType_t ticks_to_wait)
  {
    return write(reinterpret_cast<const char *>(data), size, ticks_to_wait);
  }

  esp_err_t multipart_stream::multipart_stream_handler(httpd_req_t *req, middleware_next_t next)
  {
    auto self = reinterpret_cast<multipart_stream *>(req->user_ctx);
    esp_err_t res = ESP_OK;

    if (self->_config.on_stream_start)
    {
      self->_config.on_stream_start(self->_config.ctx);
    }

    // Note the start time so we can calculate the frame rate
    int64_t send_time = esp_timer_get_time();

    ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Setting content type: %s", self->_stream_content_type.c_str());
    res = httpd_resp_set_type(req, self->_stream_content_type.c_str());
    ESP_RETURN_ON_ERROR(res, MULTIPART_STREAM_MIDDLEWARE, "Failed to set content type");

    while (true)
    {
      ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Waiting for part");
      if (!xEventGroupWaitBits(self->_event_group, PART_READY, pdTRUE, pdTRUE, portMAX_DELAY))
      {
        ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Part ready timeout");
        continue;
      }

      ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Sending part boundary");
      res = httpd_resp_send_chunk(req, self->_part_boundary.c_str(), self->_part_boundary.size());
      if (res != ESP_OK)
      {
        ESP_LOGE(MULTIPART_STREAM_MIDDLEWARE, "Failed to send boundary");
        break;
      }
      ESP_RETURN_ON_ERROR(res, MULTIPART_STREAM_MIDDLEWARE, "Failed to send boundary");

      ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Sending part headers");
      std::string part_headers =
          self->_part_content_type +
          "Content-Length: " + std::to_string(self->_part_size) + "\r\n\r\n";
      res = httpd_resp_send_chunk(req, part_headers.c_str(), part_headers.size());
      if (res != ESP_OK)
      {
        ESP_LOGE(MULTIPART_STREAM_MIDDLEWARE, "Failed to send part headers");
        break;
      }

      ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Sending part");
      res = httpd_resp_send_chunk(req, self->_part, self->_part_size);
      if (res != ESP_OK)
      {
        ESP_LOGE(MULTIPART_STREAM_MIDDLEWARE, "Failed to send part");
        break;
      }

      int64_t now = esp_timer_get_time();
      uint32_t part_time_ms = (now - send_time) / 1000;
      float fps = 1000.0 / part_time_ms;
      uint32_t part_size_kb = self->_part_size / 1024;
      send_time = now;
      ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "Part: %luKB %lums (%.1f part/s)",
               part_size_kb,
               part_time_ms,
               fps);
      xEventGroupSetBits(self->_event_group, PART_SENT);
    }

    if (self->_config.on_stream_end)
    {
      self->_config.on_stream_end(self->_config.ctx);
    }

    // This must be set so that the next part in a new stream can be sent
    xEventGroupSetBits(self->_event_group, PART_SENT);
    ESP_LOGI(MULTIPART_STREAM_MIDDLEWARE, "End of stream");
    return res;
  }

} // namespace cjf
