#ifndef DF8FF9A8_57E9_4DA8_AE65_9BA987CAF2E0
#define DF8FF9A8_57E9_4DA8_AE65_9BA987CAF2E0

#include "../web_server.h"
#include <freertos/event_groups.h>
#include <string>

namespace cjf
{

  class multipart_stream;

  struct multipart_stream_config_t
  {
    const char *boundary;
    const char *part_content_type;
    void (*on_stream_start)(void *ctx) = NULL;
    void (*on_stream_end)(void *ctx) = NULL;
    void *ctx = NULL;
  };

  class multipart_stream : public middleware_t
  {

  public:
    static constexpr const char* name = "multipart_stream";

    multipart_stream(const multipart_stream_config_t& config);
    ~multipart_stream();

    esp_err_t write(const char *data, const size_t size, TickType_t ticks_to_wait = portMAX_DELAY);
    esp_err_t write(const uint8_t *data, const size_t size, TickType_t ticks_to_wait = portMAX_DELAY);
    static esp_err_t multipart_stream_handler(httpd_req_t *req, middleware_next_t next);

  protected:
    multipart_stream(const char* name, const multipart_stream_config_t config);

  private:
    const multipart_stream_config_t _config;
    EventGroupHandle_t _event_group;
    const char *_part;
    const std::string _part_boundary;
    const std::string _part_content_type;
    ssize_t _part_size;
    int64_t _send_time;
    const std::string _stream_content_type;
  };

} // namespace cjf

#endif /* DF8FF9A8_57E9_4DA8_AE65_9BA987CAF2E0 */
