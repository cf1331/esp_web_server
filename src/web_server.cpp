#include <cjf/web_server.h>

#include <esp_http_server.h>
#include <esp_log.h>
#include <FreeRTOS.h>
#include <freertos/task.h>

namespace cjf
{

  const char *WEB_SERVER = "web_server";

  web_server::web_server(const httpd_config_t &config)
      : _config(config),
        _server(nullptr)
  {
    // Use a custom uri match function so that all uris are handled by the
    // internal _req_handler. This allows us to run multiple middleware handlers
    // for a given request.
    _config.uri_match_fn = uri_match_any;
  }

  web_server::~web_server()
  {
    if (_server)
    {
      stop();
    }
  }

  esp_err_t web_server::start()
  {
    ESP_LOGI(WEB_SERVER, "Starting web server");
    esp_err_t ret = httpd_start(&_server, &_config);
    if (ret != ESP_OK)
    {
      ESP_LOGE(WEB_SERVER, "Failed to start web server");
    }
    else
    {
      ESP_LOGI(WEB_SERVER, "Web server listening on port: %d", _config.server_port);
      _register_handler_for_method(HTTP_GET);
      _register_handler_for_method(HTTP_POST);
    }
    return ret;
  }

  esp_err_t web_server::stop()
  {
    ESP_LOGI(WEB_SERVER, "Stopping web server");
    esp_err_t ret = httpd_stop(_server);
    if (ret != ESP_OK)
    {
      ESP_LOGE(WEB_SERVER, "Failed to stop web server");
    }
    else
    {
      _server = nullptr;
      ESP_LOGI(WEB_SERVER, "Web server stopped");
    }
    return ret;
  }

  void web_server::use(middleware_t middleware)
  {
    use("/*", middleware);
  }

  void web_server::use(middleware_handler_t handler)
  {
    use("*", {nullptr, handler, nullptr});
  }

  void web_server::use(const char *path, middleware_t middleware)
  {
    const char* name = (middleware.name) ? middleware.name : "anonymous";
    ESP_LOGI(WEB_SERVER, "Using %s middleware for %s", name, path);
    _routes.push_back({path, middleware});
  }

  void web_server::use(const char *path, middleware_handler_t handler)
  {
    use(path, {nullptr, handler, nullptr});
  }

  esp_err_t web_server::_register_handler_for_method(const httpd_method_t method)
  {
    httpd_uri_t uri = {
        .uri = "/*",
        .method = method,
        .handler = _req_handler,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL};
    return httpd_register_uri_handler(_server, &uri);
  }

  esp_err_t web_server::_req_handler(httpd_req_t *req)
  {
    auto server = reinterpret_cast<web_server *>(req->user_ctx);
    auto routes = server->_routes;
    auto route = routes.begin();
    if (route == routes.end())
    {
      // No middleware registered
      return httpd_resp_send_404(req);
    }
    middleware_next_t next = [&next, &req, &routes, &route]() -> esp_err_t
    {
      while (route != routes.end() && !httpd_uri_match_wildcard(route->uri, req->uri, strlen(route->uri)))
      {
        route++;
      }
      if (route == routes.end())
      {
        return ESP_OK;
      }
      const char* name = (route->middleware.name) ? route->middleware.name : "anonymous";
      ESP_LOGI(WEB_SERVER, "Running %s middleware for %s", name, req->uri);
      vTaskDelay(pdMS_TO_TICKS(100));
      req->user_ctx = route->middleware.ctx;
      return (route++)->middleware.handler(req, next);
    };
    return next();
  }

  bool web_server::uri_match_any(const char *uri_template, const char *uri_to_match, size_t match_upto)
  {
    return true;
  }

  esp_err_t send_json_response(httpd_req_t *req, cJSON *json)
  {
    char *json_str = cJSON_PrintUnformatted(json);
    // TODO: Fix error handling
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    free(json_str);
    return ESP_OK;
  }

} // namespace cjf
