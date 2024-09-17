#ifndef AD824137_C7F6_45DB_A47C_B47B987273BA
#define AD824137_C7F6_45DB_A47C_B47B987273BA

#include <cJSON.h>
#include <esp_http_server.h>
#include <esp_err.h>
#include <functional>
#include <list>
#include <stdint.h>
#include <string>

namespace cjf
{

  using middleware_next_t = std::function<esp_err_t()>;
  // using middleware_handler_t = esp_err_t(*)(httpd_req_t *req, middleware_next_t next);
  using middleware_handler_t = std::function<esp_err_t(httpd_req_t *req, middleware_next_t next)>;

  struct middleware_t
  {
    const char* name;
    middleware_handler_t handler;
    void *ctx;
  };

  struct middleware_uri_t
  {
    const char *uri;
    middleware_t middleware;
  };

  class web_server
  {
  public:
    web_server(const httpd_config_t &config = HTTPD_DEFAULT_CONFIG());
    ~web_server();

    esp_err_t start();
    esp_err_t stop();

    void use(middleware_t middleware);
    void use(middleware_handler_t handler);
    void use(const char *path, middleware_t middleware);
    void use(const char *path, middleware_handler_t handler);

  private:
    httpd_config_t _config;
    std::list<middleware_uri_t> _routes;
    httpd_handle_t _server;

    esp_err_t _register_handler_for_method(const httpd_method_t method);

    static esp_err_t _req_handler(httpd_req_t *req);
    static bool uri_match_any(const char *uri_template, const char *uri_to_match, size_t match_upto);
  };

  esp_err_t send_json_response(httpd_req_t *req, cJSON *json);

} // namespace cjf

#endif /* AD824137_C7F6_45DB_A47C_B47B987273BA */
