#ifndef CF54F167_48E0_4F2F_B3B9_DFB9FE22459D
#define CF54F167_48E0_4F2F_B3B9_DFB9FE22459D

#include <cjf/web_server.h>
#include <cjf/middleware/multipart_stream.h>
#include <esp_camera.h>
#include <memory>
#include <freertos/event_groups.h>
#include <string>

namespace cjf
{

  class mjpeg_stream;

  struct mjpeg_stream_config_t
  {
    const char *boundary = "FRAME";
    void (*on_stream_start)(void *ctx) = NULL;
    void (*on_stream_end)(void *ctx) = NULL;
    void *ctx = NULL;
  };

  class mjpeg_stream : public multipart_stream
{
  public:
    static constexpr const char* name = "mjpeg_stream";

    mjpeg_stream(const mjpeg_stream_config_t& config);

    void write(std::shared_ptr<camera_fb_t> frame);
  };

} // namespace cjf

#endif /* CF54F167_48E0_4F2F_B3B9_DFB9FE22459D */
