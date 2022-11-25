/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "web_server.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "soc/soc_caps.h"
#if SOC_SDMMC_HOST_SUPPORTED
#include "driver/sdmmc_host.h"
#endif
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

#include <esp_https_server.h>
#include "esp_tls_crypto.h"

#include "esp_tls.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers and start an HTTPS server.
*/

//#define USE_SSL
extern rx_store_s rx_store;

static const char *TAG = "WEB_SERVER";

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (10*1024) // 20 KB
#define MAX_FILE_SIZE_STR "10KB"

typedef struct {
    char    *username;
    char    *password;
} basic_auth_info_t;

char  auth_username[16] = CONFIG_EXAMPLE_BASIC_AUTH_USERNAME;
char  auth_password[16] = CONFIG_EXAMPLE_BASIC_AUTH_PASSWORD;

#define HTTPD_401      "401 UNAUTHORIZED"           /*!< HTTP Response 401 */
#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

static char *http_auth_basic(const char *username, const char *password)
{
    int out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    asprintf(&user_info, "%s:%s", username, password);
    if (!user_info) {
        ESP_LOGE(TAG, "No enough memory for user information");
        return NULL;
    }
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    /* 6: The length of the "Basic " string
     * n: Number of bytes for a base64 encode format
     * 1: Number of bytes for a reserved which be used to fill zero
    */
    digest = calloc(1, 6 + n + 1);
    if (digest) {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    }
    free(user_info);
    return digest;
}

/* An HTTP GET handler */
static esp_err_t basic_auth_get_handler(httpd_req_t *req)
{
    char *buf = NULL;
    char *qbuf = NULL;
    size_t buf_len = 0;
    FILE *fd = NULL;
    nvs_handle_t nvs_handle;
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) {
        buf = calloc(1, buf_len);
        if (!buf) {
            ESP_LOGE(TAG, "No enough memory for basic authorization");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Authorization: %s", buf);
        } else {
            ESP_LOGE(TAG, "No auth value received");
        }


        char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
        if (!auth_credentials) {
            ESP_LOGE(TAG, "No enough memory for basic authorization credentials");
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        if (strncmp(auth_credentials, buf, buf_len)) {
            ESP_LOGE(TAG, "Not authenticated");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
            httpd_resp_send(req, NULL, 0);
        } else {
            ESP_LOGI(TAG, "Authenticated!");
            httpd_resp_set_status(req, HTTPD_200);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");

            /* Read URL query string length and allocate memory for length + 1,
             * extra byte for null termination */
            buf_len = httpd_req_get_url_query_len(req) + 1;
            if (buf_len > 1) {
                qbuf = malloc(buf_len);
                if (httpd_req_get_url_query_str(req, qbuf, buf_len) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query => %s", qbuf);
                    nvs_open_from_partition("iot_geiger", "default", NVS_READWRITE, &nvs_handle);
                    char param[64];
                    /* Get value of expected key from query string */
                    // WiFi
                    if (httpd_query_key_value(qbuf, "ssid", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => ssid=%s", param);
                        nvs_set_str(nvs_handle, "wifi_ssid", param);
                    }
                    if (httpd_query_key_value(qbuf, "wifi_password", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => wifi_password=%s", param);
                        nvs_set_str(nvs_handle, "wifi_pass", param);
                    }
                    if (httpd_query_key_value(qbuf, "wifi_mode", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => wifi_mode=%s", param);
                        if (strncmp(param, "AP", 2) == 0)
                        {
                            nvs_set_u8(nvs_handle, "wifi_mode", 0);
                        }
                        else
                        {
                            nvs_set_u8(nvs_handle, "wifi_mode", 1);
                        }
                    }
                    // web server
                    if (httpd_query_key_value(qbuf, "auth_username", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => auth_username=%s", param);
                        nvs_set_str(nvs_handle, "auth_username", param);
                    }
                    if (httpd_query_key_value(qbuf, "auth_password", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => auth_password=%s", param);
                        nvs_set_str(nvs_handle, "auth_password", param);
                    }
                    // MQTT
                    if (httpd_query_key_value(qbuf, "mqtt_broker", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => mqtt_broker=%s", param);
                        nvs_set_str(nvs_handle, "mqtt_broker", param);
                    }
                    if (httpd_query_key_value(qbuf, "mqtt_username", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => mqtt_username=%s", param);
                        nvs_set_str(nvs_handle, "mqtt_username", param);
                    }
                    if (httpd_query_key_value(qbuf, "mqtt_port", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => mqtt_port=%s", param);
                        nvs_set_str(nvs_handle, "mqtt_port", param);
                    }
                    if (httpd_query_key_value(qbuf, "mqtt_password", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => mqtt_password=%s", param);
                        nvs_set_str(nvs_handle, "mqtt_password", param);
                    }
                    if (httpd_query_key_value(qbuf, "mqtt_interval", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => mqtt_interval=%s", param);
                        nvs_set_str(nvs_handle, "mqtt_interval", param);
                    }
                    if (httpd_query_key_value(qbuf, "mqtt_protocol", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => mqtt_protocol=%s", param);
                        if (strncmp(param, "TCP", 3) == 0)
                        {
                            nvs_set_u8(nvs_handle, "mqtt_protocol", 0);
                        }
                        else
                        {
                            nvs_set_u8(nvs_handle, "mqtt_protocol", 1);
                        }
                    }
                    // Options
                    if (httpd_query_key_value(qbuf, "led", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => led=%s", param);
                        if (strncmp(param, "true", 4) == 0)
                        {
                            nvs_set_u8(nvs_handle, "led", 1);
                        }
                        else
                        {
                            nvs_set_u8(nvs_handle, "led", 0);
                        }
                    }
                    if (httpd_query_key_value(qbuf, "buzzer", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => buzzer=%s", param);
                        if (strncmp(param, "true", 4) == 0)
                        {
                            nvs_set_u8(nvs_handle, "buzzer", 1);
                        }
                        else
                        {
                            nvs_set_u8(nvs_handle, "buzzer", 0);
                        }
                    }
                    if (httpd_query_key_value(qbuf, "alarm", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => alarm=%s", param);
                        if (strncmp(param, "true", 4) == 0)
                        {
                            nvs_set_u8(nvs_handle, "alarm", 1);
                        }
                        else
                        {
                            nvs_set_u8(nvs_handle, "alarm", 0);
                        }
                    }
                    if (httpd_query_key_value(qbuf, "display", param, sizeof(param)) == ESP_OK) {
                        ESP_LOGI(TAG, "Found URL query parameter => display=%s", param);
                        if (strncmp(param, "true", 4) == 0)
                        {
                            nvs_set_u8(nvs_handle, "display", 1);
                        }
                        else
                        {
                            nvs_set_u8(nvs_handle, "display", 0);
                        }
                    }
                    nvs_close(nvs_handle);
                }
            free(qbuf);
        }

        fd = fopen("/spiffs/index.html", "r");
        if (!fd) {
             /* Respond with 500 Internal Server Error */
             httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
             return ESP_FAIL;
        }

        set_content_type_from_file(req, "/spiffs/index.html");

        /* Retrieve the pointer to scratch buffer for temporary storage */
        char *chunk = malloc(SCRATCH_BUFSIZE);
        if (!chunk) {
            ESP_LOGE(TAG, "No enough memory for index response");
            free(auth_credentials);
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        size_t chunksize;
        do {
            /* Read file in chunks into the scratch buffer */
            chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

            if (chunksize > 0) {
                /* Send the buffer contents as HTTP response chunk */
                if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                    fclose(fd);
                    ESP_LOGE(TAG, "File sending failed!");
                    /* Abort sending file */
                    httpd_resp_sendstr_chunk(req, NULL);
                    /* Respond with 500 Internal Server Error */
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                    free(auth_credentials);
                    free(buf);
                    fclose(fd);
                    return ESP_FAIL;
                }
            }

        /* Keep looping till the whole file is sent */
        } while (chunksize != 0);

        free(chunk);
        fclose(fd);
    }
        free(auth_credentials);
        free(buf);
    } else {
        ESP_LOGE(TAG, "No auth header received");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Hello\"");
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}


static httpd_uri_t basic_auth = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = basic_auth_get_handler,
};

static void httpd_register_basic_auth(httpd_handle_t server)
{
    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
    if (basic_auth_info) {
        basic_auth_info->username = auth_username;
        basic_auth_info->password = auth_password;

        basic_auth.user_ctx = basic_auth_info;
        httpd_register_uri_handler(server, &basic_auth);
    }
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char status[512];

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    snprintf(status, sizeof(status), 	"GC: %f<br>"
    					"Temperature: %f<br>"
    					"Humidity: %f<br>"
    					"PM1: %d<br>"
    					"PM2.5: %d<br>"
    					"PM10: %d<br>",
    					rx_store.current.gc_usv,
    					rx_store.current.temperature,
    					rx_store.current.humidity,
    					rx_store.current.pm1,
    					rx_store.current.pm2_5,
    					rx_store.current.pm10);
    httpd_resp_send(req, &status[0], HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t status = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /status URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /status is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * keeps it open (when requested URI is /status). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/status", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/status URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &status);
        httpd_register_basic_auth(server);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

/* Function to initialize SPIFFS*/ 
static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,   // This decides the maximum number of files that can be created on the storage
      .format_if_mount_failed = true 
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
 
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }
 
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

void web_init(void)
{
    static httpd_handle_t server = NULL;
    nvs_handle_t nvs_handle;
    size_t required_size;

    ESP_ERROR_CHECK(init_spiffs());
 
    if (nvs_open_from_partition("iot_geiger", "default",NVS_READONLY, &nvs_handle) == ESP_OK)
	{
		required_size = 16;
		if (nvs_get_str(nvs_handle, "auth_username", &auth_username[0], &required_size) != ESP_OK)
		{
			ESP_LOGE(TAG, "auth_username not in NVS");
		}
		required_size = 16;
		if (nvs_get_str(nvs_handle, "auth_password", &auth_password[0], &required_size) != ESP_OK)
		{
			ESP_LOGE(TAG, "auth_password not in NVS");
		}
		nvs_close(nvs_handle);
	}
    ESP_ERROR_CHECK(example_connect());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));

    /* Start the server for the first time */
    server = start_webserver();

}
