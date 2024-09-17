#include <cjf/middleware/mjpeg_stream.h>

namespace cjf
{

  mjpeg_stream::mjpeg_stream(const mjpeg_stream_config_t &config)
      : multipart_stream(name, {.boundary = config.boundary,
                                .part_content_type = "video/x-motion-jpeg",
                                .on_stream_start = config.on_stream_start,
                                .on_stream_end = config.on_stream_end,
                                .ctx = config.ctx})
  {
  }

  void mjpeg_stream::write(std::shared_ptr<camera_fb_t> frame)
  {
    multipart_stream::write(reinterpret_cast<char *>(frame->buf), frame->len);
  }

} // namespace cjf
