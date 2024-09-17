#ifndef A73F6FA5_4E1C_4AD9_A7E6_A52CB0B937D4
#define A73F6FA5_4E1C_4AD9_A7E6_A52CB0B937D4

#include "../web_server.h"

namespace cjf
{

  struct not_found : public middleware_t
  {
  public:
    static constexpr const char* name = "not_found";

    not_found();

  private:
    static esp_err_t not_found_handler(httpd_req_t *req, middleware_next_t next);
  };

} // namespace cjf

#endif /* A73F6FA5_4E1C_4AD9_A7E6_A52CB0B937D4 */
