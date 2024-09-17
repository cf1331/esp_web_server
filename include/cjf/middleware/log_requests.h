#ifndef D8EBA24D_944C_41DB_ACDB_AF9ABCCAC56C
#define D8EBA24D_944C_41DB_ACDB_AF9ABCCAC56C

#include "../web_server.h"

namespace cjf
{

  struct log_requests_config_t
  {
    const char *tag;
  };

  struct log_requests : public middleware_t
  {
  public:
    static constexpr const char* name = "log_requests";

    log_requests(const log_requests_config_t &config);

  private:
    const log_requests_config_t _config;
    static esp_err_t log_requests_handler(httpd_req_t *req, middleware_next_t next);
  };

} // namespace cjf

#endif /* D8EBA24D_944C_41DB_ACDB_AF9ABCCAC56C */
