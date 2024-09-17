#ifndef E1F8D50B_34E7_44A7_89D8_DB000B67EDB2
#define E1F8D50B_34E7_44A7_89D8_DB000B67EDB2

#include "../web_server.h"
#include <string>

namespace cjf
{

  struct get_files_from_storage_config_t
  {
    const char *base_path;
    size_t max_path_size;
    const char *index_filename;
    const char *cache_control;
    size_t chunk_size;
  };

  class get_files_from_storage : public middleware_t
  {
  public:
    static constexpr const char* name = "get_files_from_storage";

    get_files_from_storage(const get_files_from_storage_config_t &config);

  private:
    const get_files_from_storage_config_t _config;
    static esp_err_t get_files_from_storage_handler(httpd_req_t *req, middleware_next_t next);
  };

} // namespace cjf

#endif /* E1F8D50B_34E7_44A7_89D8_DB000B67EDB2 */
