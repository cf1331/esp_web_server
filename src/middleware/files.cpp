#include <cjf/middleware/files.h>
#include <cjf/mime.h>
#include <cjf/web_server.h>

#include <esp_log.h>
#include <stdio.h>
#include <sys/stat.h>

namespace cjf
{

  const char *FILES_MIDDLEWARE = "middleware:files";

  /**
   * @brief Extracts the path from the given URI.
   *
   * @param uri The URI from which to extract the path.
   * @return The extracted path as a string.
   */
  static std::string get_path_from_uri(const std::string uri)
  {
    return uri.substr(0, uri.find_first_of("?#"));
  }

  get_files_from_storage::get_files_from_storage(const get_files_from_storage_config_t &config)
      : middleware_t({name, get_files_from_storage_handler, this}), _config(config)
  {
  }

  esp_err_t get_files_from_storage::get_files_from_storage_handler(httpd_req_t *req, middleware_next_t next)
  {
    auto self = reinterpret_cast<get_files_from_storage *>(req->user_ctx);
    std::string uri_path = get_path_from_uri(req->uri);

    if (!uri_path.starts_with('/'))
    {
      ESP_LOGE(FILES_MIDDLEWARE, "Invalid path");
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path");
    }

    // Map the URI path to a file path
    std::string file_path = self->_config.base_path + uri_path;

    // Validate the file path is not too long for the filesystem
    if (file_path.size() > self->_config.max_path_size)
    {
      // Don't include the path in the error messages - if it's too long
      // for the filesystem it's probably too long for the logger.
      ESP_LOGE(FILES_MIDDLEWARE, "File path is too long");
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Path too long");
      return ESP_OK;
    }

    // If the name has a trailing '/', assume it's a directory and respond with
    // the index file.
    if (file_path.ends_with('/'))
    {
      file_path += self->_config.index_filename;
    }

    ESP_LOGI(FILES_MIDDLEWARE, "Responding with file \"%s\"", file_path.c_str());

    // Check that the requested file exists
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) == -1)
    {
      ESP_LOGE(FILES_MIDDLEWARE, "Failed to stat file");
      httpd_resp_send_404(req);
      return ESP_OK;
    }
    ESP_LOGI(FILES_MIDDLEWARE, "File size: %ld bytes", file_stat.st_size);

    FILE *fd = fopen(file_path.c_str(), "r");
    if (!fd)
    {
      ESP_LOGE(FILES_MIDDLEWARE, "Failed to open file");
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
      return ESP_OK;
    }

    // Set the content type based on the file extension
    set_content_type_from_path(req, file_path);

    if (self->_config.cache_control)
    {
      httpd_resp_set_hdr(req, "Cache-Control", self->_config.cache_control);
    }

    // Send the file contents in chunks
    // TODO: Consider using async http response to avoid blocking other requests
    // while the file is being sent.
    size_t max_chunk_size = self->_config.chunk_size;
    char *chunk = reinterpret_cast<char *>(malloc(max_chunk_size));
    size_t chunk_size;
    do
    {
      chunk_size = fread(chunk, 1, max_chunk_size, fd);

      if (chunk_size > 0)
      {
        if (httpd_resp_send_chunk(req, chunk, chunk_size) != ESP_OK)
        {
          fclose(fd);
          free(chunk);
          ESP_LOGE(FILES_MIDDLEWARE, "File sending failed");
          // Abort sending file
          httpd_resp_sendstr_chunk(req, NULL);
          httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
          return ESP_FAIL;
        }
      }

      // Keep looping till the whole file is sent
    } while (chunk_size != 0);

    fclose(fd);
    free(chunk);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
  }

} // namespace cjf
