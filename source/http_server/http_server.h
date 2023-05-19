#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "lwip/err.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "env.h"

#include "http_parser.h"
#include "websocket_parser.h"
#include "port_macros.h"

#define HTTP_MAX_URI_LEN            CONFIG_HTTP_MAX_URI_LEN
#define HTTP_MAX_PATH_LEN           CONFIG_HTTP_MAX_PATH_LEN
#define HTML_MAX_PATTERN_LEN        CONFIG_HTTP_MAX_PATERN_LEN
#define HTML_MAX_CHUNKED_LEN        CONFIG_HTML_MAX_CHUNKED_LEN
#define HTTP_MAX_RETRIES            5
#define HTTP_POLL_INTERVAL          4

#define HTTP_LOGD
#define HTTP_LOGE
#define HTTP_LOG

#ifndef HTTP_LOGD
#define HTTP_LOGD                   BITS_LOGD
#endif

#ifndef HTTP_LOGE
#define HTTP_LOGE                   BITS_LOGE
#endif

#ifndef HTTP_LOG
#define HTTP_LOG                    BITS_LOG
#endif

typedef struct tcp_pcb * tcp_obj_t;
struct http_server_struct;
typedef bool (*http_handle_t)(void *);
typedef enum http_method http_method_t;
typedef struct http_parser http_parser_t;

typedef struct {
    char *key;
    char *value;
} http_header_t;

typedef struct {
    http_handle_t handler;
    http_handle_t ws_init_handler;
    http_method_t method;                       /*!< The type of HTTP request, -1 if unsupported method */
    const char uri[HTTP_MAX_URI_LEN + 1];       /*!< The URI of this request (1 byte extra for null termination) */
    char *file;
    char *context;
    char *content_type;
} http_uri_t;

typedef enum {
    HTTP_CONTENT_TYPE_TEXT_HTML,
    HTTP_CONTENT_TYPE_MULTIPART_FORMDATA,
    HTTP_CONTENT_TYPE_WEBSOCKET
} http_content_type_t;

typedef enum {
    HTTP_REQ_NULL      = 0,
    HTTP_REQ_CONTENT_LENGTH,
    HTTP_REQ_CONTENT_TYPE,
    HTTP_REQ_CONNECTION,
    HTTP_REQ_SEC_WEBSOCKET_KEY
} http_req_header_t;

typedef enum {
    HTTP_RESP_HEADER,
    HTTP_RESP_CONTENT
} http_resp_state_t;

typedef enum {
    HTTP_CONN_CLOSE             = 0x00,
    HTTP_CONN_KEEP_ALIVE        = 0x01,
    HTTP_CONN_UPGRADE           = 0x02
} http_connection_t;

typedef enum {
    HTTP_WS_NULL        = 0,
    HTTP_WS_HANDSHAKE,
    HTTP_WS_OPENING
} http_websocket_state_t;

typedef struct {
    struct http_server_struct *server;      /* server */
    tcp_obj_t client_tcp;                   /* File descriptor TCP/Socket server */

    http_parser_t parser;                   /* Parser request HTTP client */
    http_parser_settings settings;          /* Parser settings cho http_parser_execute() */
    
    websocket_parser *ws_parser;
    websocket_parser_settings ws_settings;

    /**
     * @brief Cấu trúc request session đang thực thiện gửi đến server
     */
    struct {
        http_uri_t *uri;                    /* URI đang request */
        char *uri_param;
        uint16_t uri_param_len;
        http_req_header_t current_header;
        http_content_type_t content_type;   /* Content-Type */
        struct {
            char *boundary;
            uint8_t content_disposition;
            char *name;
            char *file_name;
            uint8_t content_type;
        } form_data;

        struct {
            char *key;
            http_websocket_state_t state;
        } websocket;
        size_t content_pos;         /* Vị trí đọc nội dung response */
        size_t content_len;         /* Độ dài của content */
        char *content;         /* Độ dài của content */
    } request;

    /**
     * @brief Cấu trúc response máy chủ đang thực thiện gửi đến client
     */
    struct {
        int16_t fd;
        char *context;
        char *content;              /* Con trỏ nội dung response */
        size_t content_pos;         /* Vị trí nội dung chờ response */
        size_t content_len;         /* Độ dài của content */
    } response;

    uint32_t keepalive_time_count;
    uint32_t keepalive_tick_start;
    http_connection_t connection;
    int retries;                    /* Thử kết nối lại session */
} http_session_t;

typedef struct http_server_struct {
    tcp_obj_t server_tcp;       /* File descriptor TCP/Socket server */
    ip_addr_t ip;
    http_header_t default_header[10];
    http_session_t **sess;
    struct {                /* Lưu lại các connect */
        uint8_t count;
        uint8_t max;
    } client;
    struct {                /* Cấu trúc Route cho các URI */
        uint8_t count;
        http_uri_t **index;
    } route;
} http_server_t;

extern uint16_t base64_encode(uint8_t *input_p, uint8_t *output_buf, uint16_t input_size);
err_t http_server_init(http_server_t *obj, ip_addr_t ip, uint16_t port, uint8_t max_client);
// void http_server_deinit(http_server_t *obj);
bool http_server_is_running(http_server_t *obj);
void http_server_register_uri(http_server_t *server, http_uri_t *resp);
void http_server_unregister_uri(http_server_t *server, http_uri_t *resp);
size_t http_server_get_available_mem(http_session_t *sess);
int16_t http_server_write(http_session_t *sess, char *content, size_t content_len);

#endif