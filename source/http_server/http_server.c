
#include "lwip/memp.h"
#include "mem.h"
#include "http_server.h"
#include "fs.h"
#include "sha1.h"
#include "ezform.h"

#define TCP_DEFAULT_KEEPALIVE_IDLE_SEC          7200 // 2 hours
#define TCP_DEFAULT_KEEPALIVE_INTERVAL_SEC      75   // 75 sec
#define TCP_DEFAULT_KEEPALIVE_COUNT             3    // fault after 9 failures
#define JOIN_KEY_SHA1	                        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

static err_t http_server_tcp_accept_cb(void *arg, tcp_obj_t newpcb, err_t err);
static err_t http_server_tcp_sent(void *arg, tcp_obj_t tpcb, u16_t len);
static err_t http_server_tcp_recv(void *arg, tcp_obj_t cl_pcb, struct pbuf *p, err_t err);
static err_t http_server_tcp_poll(void *arg, tcp_obj_t pcb);
static err_t http_server_tcp_close(tcp_obj_t cl_pcb, http_session_t *cl_sess, bool abort_conn);
static void http_server_tcp_error(void *arg, err_t err);

static int http_server_parser_cb_url(http_parser_t *parser, const char *at, size_t length);
static int http_server_parser_cb_header_field(http_parser_t *parser, const char *at, size_t length);
static int http_server_parser_cb_header_value(http_parser_t *parser, const char *at, size_t length);
static int http_server_parser_cb_headers_complete(http_parser_t *parser);
static int http_server_parser_cb_on_body(http_parser_t *parser, const char *at, size_t length);
static int http_server_parser_cb_no_body(http_parser_t *parser);

int websocket_frame_header(websocket_parser * parser);
int websocket_frame_body(websocket_parser * parser, const char *at, size_t size);
int websocket_frame_end(websocket_parser * parser);

static void http_server_tcp_keepalive(http_session_t *obj, uint16_t idle_sec, uint16_t intv_sec, uint8_t count);
static void http_server_session_free(http_session_t *sess);
static bool http_server_request_handler(http_session_t *sess);
static bool http_server_response_exe(http_session_t *sess);
static void http_server_free_request(http_session_t *sess);
static http_session_t *http_server_session_new(http_server_t *server);

/* -----------------------------------------> các hàm Callback TCP <----------------------------------------- */
/**
 * @brief Hàm callback TCP khi có kết nối mới
 * 
 * @param arg tham số bổ sung
 * @param newpcb pcb của connect
 * @param err mã lỗi
 * @return err_t
 */
static err_t http_server_tcp_accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err) {
    http_server_t *server = (http_server_t *)arg;
    http_session_t *this_sess = 0;

    /* Giới hạn kết nối TCP */
    if(server->client.count >= server->client.max) {
        return 0;
    }
    this_sess = http_server_session_new(server);
    HTTP_LOGD(__FILE__":%d HTTP accept: sess=0x%X pcb=0x%08X\r\n", __LINE__, this_sess, newpcb);
    if(this_sess) {
        this_sess->parser.data = (void *)this_sess;
        this_sess->client_tcp = newpcb;
        this_sess->server = server;

        /**
         * @brief Cấu hình TCP callback
         */
        tcp_arg(newpcb, this_sess);
        tcp_setprio(newpcb, TCP_PRIO_MIN);
        tcp_recv(newpcb, http_server_tcp_recv);
        tcp_sent(newpcb, http_server_tcp_sent);
        tcp_err(newpcb, http_server_tcp_error);
        tcp_poll(newpcb, http_server_tcp_poll, 4);
        tcp_nagle_disable(newpcb);
        newpcb->so_options |= SOF_KEEPALIVE;

        /**
         * @brief Khởi tạo Parser
         */
        http_parser_init(&this_sess->parser, HTTP_REQUEST);
        http_parser_settings_init(&this_sess->settings);

        this_sess->settings.on_url              = http_server_parser_cb_url;
        this_sess->settings.on_header_field     = http_server_parser_cb_header_field;
        this_sess->settings.on_header_value     = http_server_parser_cb_header_value;
        this_sess->settings.on_headers_complete = http_server_parser_cb_headers_complete;
        this_sess->settings.on_body             = http_server_parser_cb_on_body;
        this_sess->settings.on_message_complete = http_server_parser_cb_no_body;

        server->client.count++;
        return ERR_OK;
    }

    /* Đóng kết nối */
    tcp_close(newpcb);
    return ERR_MEM;
}

/**
 * @brief Hàm call back khi nhận TCP
 * 
 * @param arg Đối số bổ xung
 * @param cl_pcb pcb TCP của client
 * @param p pbuf dữ liệu nhận TCP
 * @param err lỗi
 * @return err_t 
 */
static err_t http_server_tcp_recv(void *arg, tcp_obj_t cl_pcb, struct pbuf *p, err_t err) {
    char *ptr = 0;
    size_t size_parsed = 0;
    http_session_t *sess = (http_session_t *)arg;

    if ((err != ERR_OK) || (p == NULL) || (sess == NULL)) {
        if (p != NULL) {    /* Nếu nhận được frame trống từ client thì đóng kết nối */
            tcp_recved(cl_pcb, p->tot_len);     /* Thông báo với TCP đã lấy dữ liệu */
            pbuf_free(p);
        }
        if((sess != NULL) && (err != ERR_OK) || (p == NULL)) {
            // HTTP_LOGD(__FILE__":%d BKPT close p null\r\n", __LINE__);
            http_server_tcp_close(cl_pcb, sess, false);
        }
        return ERR_OK;
    }

    // printf(__FILE__":%d recv tcp: tot=%d len=%d\r\n", __LINE__, p->tot_len, p->len);
    // for(uint16_t idx_char = 0; idx_char < 20; idx_char++) {
    //     if((((const char *)p->payload)[idx_char] >= ' ') && (((const char *)p->payload)[idx_char] < '~')) {
    //         printf("%c", ((const char *)p->payload)[idx_char]);
    //     }
    //     else {
    //         printf("{%02X}", ((const char *)p->payload)[idx_char]);
    //     }
    // }
    // printf("\r\n");

    if(sess->connection == HTTP_CONN_UPGRADE) {
        if(sess->request.websocket.state == HTTP_WS_OPENING) {

            size_parsed = websocket_parser_execute(sess->ws_parser, &sess->ws_settings, (const char *)p->payload, p->tot_len);
            // free(sess->ws_parser);
            if(http_server_response_exe(sess)) {
            }
        }
    }
    else {
        size_parsed = http_parser_execute(&sess->parser, &sess->settings, p->payload, p->tot_len);      /* Giải mã http */
    }
    tcp_recved(cl_pcb, p->tot_len);     /* Thông báo với TCP đã lấy dữ liệu */
    pbuf_free(p);
    return ERR_OK;
}

/**
 * @brief Hàm này sẽ được gọi sau khi hoàn thành gửi gói TCP
 * 
 * @param arg Đối số bổ xung
 * @param pcb pcb TCP của client
 * @param len Độ dài dữ liệu đã gửi
 * @return err_t 
 */
err_t http_server_tcp_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    /* Lấy đối tượng session hiện tại */
    http_session_t *sess = (http_session_t *)arg;    
    HTTP_LOGD(__FILE__":%d HTTP Server sent: sess=0x%08X pcb=0x%08X pcbs=0x%08X\r\n", __LINE__, sess, pcb, sess->client_tcp);
    if(http_server_response_exe(sess) == true) {
        if(sess->connection == HTTP_CONN_KEEP_ALIVE) {
            // HTTP_LOGD(__FILE__":%d BKPT close sent\r\n", __LINE__);
            http_server_tcp_close(sess->client_tcp, sess, 0);
        }
        if(sess->connection != HTTP_CONN_UPGRADE) {
            http_server_free_request(sess);
        }
    }
    sess->retries = 0;
}

/**
 * @brief Hàm này được gọi theo định thời của lwip, config bằng tcp_poll
 * 
 * @param arg Đối số bổ xung 
 * @param pcb pcb TCP của client
 * @return 
 */
static err_t http_server_tcp_poll(void *arg, struct tcp_pcb *pcb) {
    http_session_t *sess = (http_session_t *)arg;
    HTTP_LOGD(__FILE__":%d HTTP Server sent: sess=0x%08X pcb=0x%08X\r\n", __LINE__, sess, pcb);

    if (sess == NULL) {
        // HTTP_LOGD(__FILE__":%d BKPT sess nul in poll\r\n", __LINE__);
        http_server_tcp_close(pcb, 0, false);
        return ERR_OK;
    } else {
        if(sess->connection == HTTP_CONN_CLOSE) {
            if (sess->retries++ >= HTTP_MAX_RETRIES) {
                // HTTP_LOGD(__FILE__":%d BKPT max retries\r\n", __LINE__);
                http_server_tcp_close(pcb, sess, false);
                return ERR_OK;
            }
        }        
        if (sess->request.uri) {
            if(http_server_response_exe(sess) == true) {
                if(sess->connection != HTTP_CONN_UPGRADE) {
                    http_server_free_request(sess);
                }
            }
        }
    }

    return ERR_OK;
}

/**
 * @brief  Hàm callback khi lỗi TCP
 * @param  arg: pointer on argument parameter
 * @param  err: not used
 * @retval None
 */
static void http_server_tcp_error(void *arg, err_t err) {
    LWIP_UNUSED_ARG(err);
    http_session_t *sess = (http_session_t *)arg;
    HTTP_LOGD(__FILE__":%d BKPT tcp err %d\r\n", __LINE__, err);
    http_server_tcp_close(sess->client_tcp, sess, 0);
}

/* -----------------------------------------> các hàm Callback Parser HTTP server <----------------------------------------- */
static int http_server_parser_cb_url(http_parser_t *parser, const char *at, size_t length) {
    http_session_t *sess = (http_session_t *)parser->data;
    http_server_t *server = sess->server;
    struct tcp_pcb *this_cl_pcb = (struct tcp_pcb *)(sess->client_tcp);
    int uri_multi = -1;
    char *compare_stop = 0;

    /* --- DEBUG --- */
    // printf(__FILE__":%d CB URL: ", __LINE__);
    // for(uint16_t idx_char = 0; idx_char < length; idx_char++) {
    //     printf("%c", at[idx_char]);
    // }
    // printf("\r\n");
    /* --- DEBUG --- */
    
    if(sess->request.uri) {
        return -1;
    }
    compare_stop = strchr(at + 1, '?');
    if((compare_stop == 0) || ((compare_stop - at) > 256)) {
        compare_stop = at + length;
    }

    for(uint8_t idx_resp_call = 0; idx_resp_call < server->route.count; idx_resp_call++){
        if((uint32_t)(server->route.index[idx_resp_call]) == 0) break;
        if(memcmp(server->route.index[idx_resp_call]->uri, "/*", 2) == 0) uri_multi = idx_resp_call;
        if(parser->method == server->route.index[idx_resp_call]->method) {
            if((compare_stop - at) >= strlen(server->route.index[idx_resp_call]->uri)) {
                /* Kiểm tra URI request với những route có trên server */
                if(memcmp(server->route.index[idx_resp_call]->uri, at, compare_stop - at) == 0) {
                    sess->request.uri = server->route.index[idx_resp_call];
                    if(sess->request.uri->method == HTTP_GET) {
                        /* Lấy param trong URI nếu có, định dạng: URL?key=value.... */
                        if(*compare_stop == '?') {
                            /* --- DEBUG --- */
                            // HTTP_LOGD(__FILE__":%d Have param in URI\r\n", __LINE__);
                            /* --- DEBUG --- */
                            length -= strlen(server->route.index[idx_resp_call]->uri) + 1;
                            sess->request.uri_param = compare_stop + 1;
                            sess->request.uri_param_len = length;
                        }
                    }
                    else {
                        sess->request.uri_param = 0;
                        sess->request.uri_param_len = 0;
                    }
                    return 0;
                }
            }
        }
    }

    sess->request.uri = 0;
    if(uri_multi != -1) {
        sess->request.uri = server->route.index[uri_multi];
        return 0;
    }
    return 0;
}
static int http_server_parser_cb_header_field(http_parser_t *parser, const char *at, size_t length){
    http_server_t *server = (http_server_t *)(((http_session_t *)parser->data)->server);
    http_session_t *sess = (http_session_t *)parser->data;
    struct tcp_pcb *this_cl_pcb = (struct tcp_pcb *)(sess->client_tcp);

    if(memcmp(at, "Content-Length", 14) == 0) {
        sess->request.current_header = HTTP_REQ_CONTENT_LENGTH;
    }
    else if(memcmp(at, "Content-Type", 12) == 0) {
        sess->request.current_header = HTTP_REQ_CONTENT_TYPE;
    }
    else if(memcmp(at, "Connection", 10) == 0) {
        sess->request.current_header = HTTP_REQ_CONNECTION;
    }
	if(memcmp(at, "Sec-WebSocket-Key", 17) == 0) {
		sess->request.current_header = HTTP_REQ_SEC_WEBSOCKET_KEY;
	}
    return 0;
}
static int http_server_parser_cb_header_value(http_parser_t *parser, const char *at, size_t length){
    http_server_t *server = (http_server_t *)(((http_session_t *)parser->data)->server);
    http_session_t *sess = (http_session_t *)parser->data;
    struct tcp_pcb *this_cl_pcb = (struct tcp_pcb *)(sess->client_tcp);

    switch(sess->request.current_header) {
        case HTTP_REQ_NULL : {
            break;
        }
        case HTTP_REQ_CONTENT_LENGTH : {
            sess->request.content_len = Bits_String_ToInt((char *)at);
            sess->request.content_pos = 0;
            break;
        }
        case HTTP_REQ_CONTENT_TYPE : {
            if(memcmp(at, "text/", 5) == 0) {
                sess->request.content_type = HTTP_CONTENT_TYPE_TEXT_HTML;
            }
            else if(memcmp(at, "multipart/", 10) == 0) {
                sess->request.content_type = HTTP_CONTENT_TYPE_MULTIPART_FORMDATA;      /* multipart/form-data */
                /* Lấy Boundary */
                char *start_boundary = strstr(at, "boundary");
                if(start_boundary) {
                    start_boundary += 9;    /* boundary= */
                    char *stop_boundary = strstr(start_boundary, "\r\n");
                    if(stop_boundary) {
                        size_t boundary_length = stop_boundary - start_boundary;
                        /**
                         * @brief Free boundary nếu nó đang có sẵn
                         */
                        if(sess->request.form_data.boundary) {
                            free(sess->request.form_data.boundary);
                            sess->request.form_data.boundary = 0;
                        }
                        /**
                         * @brief Lấy boundary mới
                         */
                        sess->request.form_data.boundary = malloc(boundary_length + 1);
                        if(sess->request.form_data.boundary) {
                            memcpy(sess->request.form_data.boundary, start_boundary, boundary_length);
                            sess->request.form_data.boundary[boundary_length] = 0;
                        }
                    }
                }
            }
            break;
        }
        case HTTP_REQ_CONNECTION : {
            if((memcmp(at, "keep-alive", 10) == 0) || (memcmp(at, "Keep-Alive", 10) == 0)) {
                sess->connection = HTTP_CONN_KEEP_ALIVE;
            }
            else if((memcmp(at, "close", 5) == 0) || (memcmp(at, "Close", 5) == 0)) {
                sess->connection = HTTP_CONN_CLOSE;
            }
            else if((memcmp(at, "Upgrade", 7) == 0) || (memcmp(at, "upgrade", 7) == 0)) {
                sess->connection = HTTP_CONN_UPGRADE;
            }
            else {
                sess->connection = HTTP_CONN_CLOSE;
            }

            break;
        }
        case HTTP_REQ_SEC_WEBSOCKET_KEY : {
            sess->request.websocket.state = HTTP_WS_HANDSHAKE;
            sess->request.websocket.key = (char *)malloc(length + 36 + 1);
            if(sess->request.websocket.key) {
                memcpy(sess->request.websocket.key, at, length);
                memcpy(sess->request.websocket.key + length, JOIN_KEY_SHA1, 36);
                sess->request.websocket.key[length + 36] = 0;
            }
            break;
        }
        default: {
            break;
        }
    }
    /* Reset header */
    sess->request.current_header = HTTP_REQ_NULL;
    return 0;
}
static int http_server_parser_cb_headers_complete(http_parser_t *parser){
    http_session_t *sess = (http_session_t *)parser->data;
    struct tcp_pcb *this_cl_pcb = sess->client_tcp;
    if(sess->connection == HTTP_CONN_KEEP_ALIVE) {
        http_server_tcp_keepalive(sess, TCP_DEFAULT_KEEPALIVE_IDLE_SEC, TCP_DEFAULT_KEEPALIVE_INTERVAL_SEC, TCP_DEFAULT_KEEPALIVE_COUNT);
    }
    sess->request.current_header = HTTP_REQ_NULL;
    return 0;
}
static int http_server_parser_cb_on_body(http_parser_t *parser, const char *at, size_t length) {
    http_session_t *sess = (http_session_t *)parser->data;
    
    // if(sess->request.uri) HTTP_LOGD(__FILE__":%d cb on body uri: %s\r\n", __LINE__, sess->request.uri->uri);

    if(sess->request.uri) {
        sess->request.content = (char *)at;
        sess->request.content_len = length;
        sess->request.content_pos = 0;
        http_server_request_handler(sess);
    }
    return 0;
}
static int http_server_parser_cb_no_body(http_parser_t *parser) {
    http_session_t *sess = (http_session_t *)parser->data;
    if(sess->request.uri) {
        http_server_request_handler(sess);
    }
    return 0;
}

/* -----------------------------------------> các hàm Callback Parser Websocket <----------------------------------------- */
int websocket_frame_header(websocket_parser * parser) {
    return 0;
}
int websocket_frame_body(websocket_parser * parser, const char *at, size_t size) {
    http_session_t *sess = (http_session_t *)(parser->data);
    char *content = (char *)at;
    if(sess->ws_parser->flags & WS_HAS_MASK) {
        // HTTP_LOGD(__FILE__":%d decode websocket\r\n", __LINE__);
        websocket_parser_decode(content, at, size, parser);
    }
    // HTTP_LOGD(__FILE__":%d websocket data: len=%d\r\n", __LINE__, size);
    sess->request.content = at;
    sess->request.content_len = size;
    sess->request.content_pos = 0;

    if(sess->request.uri) {
        if(sess->request.uri->handler) {
            sess->request.uri->handler(sess);
        }
    }
    return 0;
}
int websocket_frame_end(websocket_parser * parser) {
    http_session_t *sess = (http_session_t *)(parser->data);
    if(parser) {
        free(parser);
        sess->ws_parser = 0;
        /* Khởi tạo parser mới */
        sess->ws_parser = (websocket_parser *)malloc(sizeof(websocket_parser));
        sess->ws_parser->data = (void *)sess;
        websocket_parser_init(sess->ws_parser);
    }
}

/* -----------------------------------------> các hàm HTTP server <----------------------------------------- */
/**
 * @brief Khởi tạo server HTTP
 * 
 * @param dev đối tượng server
 * @param port port
 * @return err_t 
 */
err_t http_server_init(http_server_t *obj, ip_addr_t ip, uint16_t port, uint8_t max_client) {
    err_t err = ERR_MEM;

    HTTP_LOGD(__FILE__":%d HTTP Server init port=%d ", __LINE__, port);

    if(obj == 0) {
        printf("Error Server ptr!\r\n");
        return err;
    }
    obj->client.max = max_client;
    obj->client.count = 0;
    obj->ip = ip;
    obj->server_tcp = tcp_new();
    if (obj->server_tcp) {
    	struct tcp_pcb *lpcb = NULL;
        obj->server_tcp->so_options |= SOF_REUSEADDR;
        err = tcp_bind(obj->server_tcp, &obj->ip, port);
        lpcb = obj->server_tcp;
        if (err == ERR_OK) {
            obj->server_tcp = tcp_listen_with_backlog(obj->server_tcp, 5);
            if (obj->server_tcp == 0) {
                tcp_close(obj->server_tcp);
                obj->server_tcp = 0;
                printf("Error listen!\r\n");
                return ERR_MEM;
            }
            port = obj->server_tcp->local_port;
            tcp_accept(obj->server_tcp, http_server_tcp_accept_cb);
            tcp_arg(obj->server_tcp, obj);
            printf("OK!\r\n");
            return ERR_OK;
        }
        else {
        	memp_free(MEMP_TCP_PCB, lpcb);
            tcp_close(obj->server_tcp);
            obj->server_tcp = 0;
            printf("Error bind %d!\r\n", err);
            return ERR_MEM;
        }
    }
    return ERR_OK;
}

// void http_server_deinit(http_server_t *server) {
//     if(server == 0) return;
//     /* Đóng server */
//     HTTP_LOGD(__FILE__":%d HTTP Server close pcb=0x%08X ", __LINE__, server->server_tcp);
//     if(server->server_tcp) {
//         if(tcp_close(server->server_tcp) != ERR_OK) {
//             printf("Fail\r\n");
//             return;
//         }
//         printf("OK\r\n");
//         server->server_tcp = 0;
//     }
//     if(server->sess) {
//         for(uint8_t idx_sess = 0; server->sess[idx_sess] != 0; idx_sess++) {
//             http_server_tcp_close(server->sess[idx_sess]->client_tcp, server->sess[idx_sess], false);
//             // http_server_session_free(server->sess[idx_sess]);
//             if(server->sess == 0) break;
//         }
//     }
//     HTTP_LOGD("Done!\r\n");
//     server->client.count = 0;
// }
/**
 * @brief Lấy số Byte có thể gửi qua TCP
 * @param sess session đang xử lý
 */
size_t http_server_get_available_mem(http_session_t *sess) {
    struct tcp_pcb *this_cl_pcb = sess->client_tcp;
    return tcp_sndbuf(this_cl_pcb);
}
/**
 * @brief Kiểm tra server đang hoạt động
 * 
 * @param obj 
 * @return true 
 * @return false 
 */
bool http_server_is_running(http_server_t *obj) {
    if(obj == 0) return false;
    if(obj->server_tcp != 0) {
        return true;
    }
    return false;
}
/**
 * @brief Hàm đưa dữ liệu vào buffer, chuẩn bị gửi cho client.
 * 
 * @param sess 
 * @param content 
 * @param content_len 
 */
int16_t http_server_write(http_session_t *sess, char *content, size_t content_len) {
    if(sess->response.content == 0) {
        sess->response.content = malloc(content_len);
        if(sess->response.content == 0) return -1;
        memset(sess->response.content, 0, content_len);
        sess->response.content_len = content_len;
        memcpy(sess->response.content, content, content_len);
    }
    else {
        sess->response.content = realloc(sess->response.content, content_len + sess->response.content_len);
        if(sess->response.content == 0) return -1;
        memcpy(sess->response.content + sess->response.content_len, content, content_len);
        sess->response.content_len += content_len;
    }
    return 0;
}
/**
 * @brief Thực thi Response
 * 
 * @param sess 
 * @return true Đã thực thi xong Response
 * @return false Chưa thực thi xong Response
 */
static bool http_server_response_exe(http_session_t *sess) {
    int16_t err = 0;    
    http_server_t *server = sess->server;
    struct tcp_pcb *this_cl_pcb = (struct tcp_pcb *)(sess->client_tcp);
    HTTP_LOGD(__FILE__":%d Response client: sess=0x%08X pcb=0x%08X\r\n", __LINE__, sess, this_cl_pcb);
    size_t size_content_can_send = tcp_sndbuf(this_cl_pcb);    /* Kiểm tra kích thước dữ liệu có thể gửi đi */
    if(sess->response.content) {
        if(size_content_can_send > (sess->response.content_len - sess->response.content_pos)) {
            size_content_can_send = (sess->response.content_len - sess->response.content_pos);
        }
        do {
            err = tcp_write(this_cl_pcb, sess->response.content + sess->response.content_pos, size_content_can_send, TCP_WRITE_FLAG_COPY);
            if (err == ERR_MEM) {
                if ((tcp_sndbuf(this_cl_pcb) == 0) ||
                    (tcp_sndqueuelen(this_cl_pcb) >= TCP_SND_QUEUELEN)) {
                    /* no need to try smaller sizes */
                    size_content_can_send = 1;
                } else {
                    size_content_can_send /= 2;
                }
            }
        } while ((err == ERR_MEM) && (size_content_can_send > 1));

        if(err == 0) {
            tcp_output(this_cl_pcb);
            sess->response.content_pos += size_content_can_send;
            if(sess->response.content_pos >= sess->response.content_len) {
                sess->response.content_pos = 0;
                sess->response.content_len = 0;
                if(sess->response.content) {
                    if(sess->response.content == sess->response.context) {
                        sess->response.context = 0;
                    }
                    else {
                        free(sess->response.content);
                    }
                }
                sess->response.content = 0;
            }
        }
        tcp_nagle_enable(this_cl_pcb);
        HTTP_LOGD(__FILE__":%d Response client out: sess=0x%08X pcb=0x%08X\r\n", __LINE__, sess, this_cl_pcb);
    }
    else {
        if(sess->response.fd > 0) {
            uint32_t size_can_read = favailable(sess->response.fd);
            if(size_can_read == 0) {
                HTTP_LOG(__FILE__":%d Close file %s : %d\r\n", __LINE__, sess->request.uri->file, sess->response.fd);
                fclose(sess->response.fd);
                sess->response.fd = 0;
                return true;
            }
            if(size_can_read > 1024) size_can_read = 1024;
            sess->response.content = malloc(size_can_read);
            HTTP_LOGD(__FILE__":%d Response client malloc: %d\r\n", __LINE__, system_get_free_heap_size());
            if(sess->response.content == 0) {
                sess->response.content_len = 0;
                sess->response.content_pos = 0;
                return false;
            }
            fread(sess->response.fd, sess->response.content, size_can_read);
            sess->response.content_len = size_can_read;
            sess->response.content_pos = 0;
        }
        else if(sess->response.context) {
            sess->response.content = sess->response.context;
            sess->response.content_len = ((uint32_t)sess->response.content[0]);
            sess->response.content_len |= ((uint32_t)sess->response.content[1]) << 8;
            sess->response.content_len |= ((uint32_t)sess->response.content[2]) << 16;
            sess->response.content_len |= ((uint32_t)sess->response.content[3]) << 32;
            sess->response.content_len += 4;
            /* Bỏ qua 4 Byte kích thước */
            sess->response.content_pos = 4;
        }
        else {
            return true;
        }
    }
    return false;
}
/**
 * @brief Xử lý Request từ Client đến Server, config các Response theo Request
 * 
 * @param sess 
 * @return true 
 * @return false 
 */
static bool http_server_request_handler(http_session_t *sess) {
    struct tcp_pcb *this_cl_pcb = (struct tcp_pcb *)(sess->client_tcp);
    HTTP_LOGD(__FILE__":%d Client request: sess=0x%08X pcb=0x%08X\r\n", __LINE__, sess, this_cl_pcb);
    uint32_t file_size = 0;
    #define __DEFAULT_HDR_PART 360
    if(sess->connection == HTTP_CONN_UPGRADE) {
        if(sess->request.websocket.state == HTTP_WS_HANDSHAKE) {
            uint8_t * websocket_key_hex = (uint8_t *)malloc(21);
            if(websocket_key_hex) {
                char *websocket_key_str = (char *)malloc(41);
                if(websocket_key_str) {
                    /* Mã hóa SHA-1 */
                    SHA1_CTX shactx;
                    SHA1Init(&shactx);
                    SHA1Update(&shactx, sess->request.websocket.key, strlen(sess->request.websocket.key));
                    free(sess->request.websocket.key);
                    sess->request.websocket.key = 0;
                    SHA1Final(websocket_key_hex, &shactx);
                    /* Mã hóa base64 */
                    websocket_key_str[base64_encode(websocket_key_hex, websocket_key_str, 20)] = 0;
                    http_server_write(sess, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ", 97);
                    http_server_write(sess, websocket_key_str, strlen(websocket_key_str));
                    http_server_write(sess, "\r\n\r\n", 4);
                    free(websocket_key_str);
                    /* Cấu hình parser websocket */
                    websocket_parser_settings_init(&sess->ws_settings);
                    sess->ws_settings.on_frame_header = websocket_frame_header;
                    sess->ws_settings.on_frame_body   = websocket_frame_body;
                    sess->ws_settings.on_frame_end    = websocket_frame_end;
                    sess->ws_parser = (websocket_parser *)malloc(sizeof(websocket_parser));
                    sess->ws_parser->data = (void *)sess;
                    websocket_parser_init(sess->ws_parser);
                    /* Chuyển trạng thái websocket */
                    sess->request.websocket.state = HTTP_WS_OPENING;
                    // HTTP_LOGD(__FILE__":%d Websocket openning\r\n", __LINE__);
                    if(sess->request.uri->ws_init_handler) {
                        sess->request.uri->ws_init_handler(sess);
                    }
                }
                free(websocket_key_hex);
            }
        }
    }
    else {
        /* ###### Lấy độ dài tất cả content từ handler và file sau đó tạo header ###### */
        /* Tạo phân vùng cho Header */
        sess->response.content = malloc(__DEFAULT_HDR_PART);
        if(sess->response.content == 0) return false;
        memset(sess->response.content, '0', __DEFAULT_HDR_PART);
        sess->response.content_len = __DEFAULT_HDR_PART;
        sess->response.content_pos = 0;
        /* Gọi đến handler
        Trong handler người dùng sẽ dùng hàm http_server_write để ghi dữ liệu vào bộ đệm con trỏ response.content,
        độ dài của bộ đệm là response.content_len và vị trí hiện tại đang thực thi response là response.content_pos
        */
        if(sess->request.uri->handler) {
            sess->request.uri->handler(sess);
        }
        sess->response.content_len -= __DEFAULT_HDR_PART;
        /* Lấy fd cho response và kích thước file */
        if(sess->request.uri->file) {
            sess->response.fd = fopen(sess->request.uri->file, "r");
            if(sess->response.fd > 0) {
                HTTP_LOG(__FILE__":%d Open file %s : %d\r\n", __LINE__, sess->request.uri->file, sess->response.fd);
                file_size = fsize(sess->response.fd);
            }
            else {
                HTTP_LOGE(__FILE__":%d Open file=%s fd=%d\r\n", __LINE__, sess->request.uri->file, sess->response.fd);
            }
        }
        else if(sess->request.uri->context) {
            HTTP_LOG(__FILE__":%d Response by context\r\n", __LINE__);
            file_size = ((uint32_t)sess->request.uri->context[0]);
            file_size |= ((uint32_t)sess->request.uri->context[1]) << 8;
            file_size |= ((uint32_t)sess->request.uri->context[2]) << 16;
            file_size |= ((uint32_t)sess->request.uri->context[3]) << 32;
            sess->response.context = sess->request.uri->context;
        }
        /* Tạo header */
        memset(sess->response.content, 0, __DEFAULT_HDR_PART);

        strcpy(sess->response.content,  "HTTP/1.1 200 OK\r\n"
                                        "Connection: keep-alive\r\n"
                                        "Keep-Alive: timeout=5\r\n"
                                        "Content-Type: ");
        memcpy(sess->response.content + 78, sess->request.uri->content_type, strlen(sess->request.uri->content_type));
        memcpy(sess->response.content + 78 + strlen(sess->request.uri->content_type), "\r\n", 2);
        for(uint8_t idx_hdr = 0; idx_hdr < 10; idx_hdr++) {
            if((sess->server->default_header[idx_hdr].key == 0) || (sess->server->default_header[idx_hdr].value == 0)) {
                break;
            }
            strcat(sess->response.content, sess->server->default_header[idx_hdr].key);
            strcat(sess->response.content, ": ");
            strcat(sess->response.content, sess->server->default_header[idx_hdr].value);
            strcat(sess->response.content, "\r\n");
        }
        if(strlen(sess->response.content) >= 350){
            return false;
        }
        sprintf(sess->response.content + strlen(sess->response.content), "Content-Length: %d\r\n\r\n", file_size + sess->response.content_len);

        uint16_t hdr_len = strlen(sess->response.content);
        for(uint16_t idx_byte = 0; idx_byte < sess->response.content_len; idx_byte++) {
            sess->response.content[hdr_len + idx_byte] = sess->response.content[__DEFAULT_HDR_PART + idx_byte];
        }
        sess->response.content_len += hdr_len;
        sess->response.content_pos = 0;
        /* Free phần ô nhớ dư */
        sess->response.content = realloc(sess->response.content, sess->response.content_len);
    }
    /* Thực thi response */
    if(http_server_response_exe(sess) == true) {
        if(sess->connection != HTTP_CONN_UPGRADE) {
            http_server_free_request(sess);
        }
    }
}
/**
 * @brief  Hàm callback khi có kết nối TCP bị đóng.
 * @param  tcp_pcb pointer on the tcp connection
 * @retval None
 */
static err_t http_server_tcp_close(struct tcp_pcb *cl_tpcb, http_session_t *sess, bool abort_conn) {
    HTTP_LOGD(__FILE__":%d HTTP close: sess=0x%08X pcb=0x%08X\r\n", __LINE__, sess, sess->client_tcp);
    /* Giải phóng bộ nhớ của session này */
    if(cl_tpcb != NULL) {
        /* Xóa tất cả callback của client */
        tcp_arg(cl_tpcb, NULL);
        tcp_sent(cl_tpcb, NULL);
        tcp_recv(cl_tpcb, NULL);
        tcp_err(cl_tpcb, NULL);
        tcp_poll(cl_tpcb, NULL, 0);
    }
    if(cl_tpcb != NULL) {
        /* Đóng kết nối */
        if (abort_conn) {
            tcp_abort(cl_tpcb);
            return ERR_OK;
        }
        else if (tcp_close(cl_tpcb) != ERR_OK) {
            tcp_poll(cl_tpcb, http_server_tcp_poll, HTTP_POLL_INTERVAL);
            return ERR_INPROGRESS;
        }
        if(sess->server->client.count) sess->server->client.count--;
    }
    if(sess != 0) {
        http_server_session_free(sess);
        return ERR_OK;
    }
    return ERR_OK;
}
static void http_server_free_request(http_session_t *sess) {
    // HTTP_LOGD(__FILE__":%d HTTP free request\r\n", __LINE__);
    /* Reset request */
    sess->request.uri = 0;
    sess->request.content = 0;
    sess->request.content_len = 0;
    sess->request.content_pos = 0;
    sess->request.current_header = HTTP_REQ_NULL;
}
static void http_server_tcp_keepalive(http_session_t *obj, uint16_t idle_sec, uint16_t intv_sec, uint8_t count) {
    tcp_obj_t tcp = obj->client_tcp;
    
    if (idle_sec && intv_sec && count) {
        tcp->so_options |= SOF_KEEPALIVE;
        tcp->keep_idle = (uint32_t)1000 * idle_sec;
        tcp->keep_intvl = (uint32_t)1000 * intv_sec;
        tcp->keep_cnt = count;
    }
    else
        tcp->so_options &= ~SOF_KEEPALIVE;
}
static http_session_t *http_server_session_new(http_server_t *server) {
    /* Tạo trạng thái session mới */
    uint8_t sess_count = 0;
    http_session_t *new_sess = (http_session_t *)malloc(sizeof(http_session_t));
    if(new_sess) {
        // if(server->sess == 0) {
        //     server->sess = (http_session_t *)malloc(sizeof(void (*)) * 2);
        // }
        // else {
        //     /* Đếm số lượng session hiện tại */
        //     for(sess_count = 0; server->sess[sess_count] != 0; sess_count++) {
        //     }
        //     /* Cấp phát lại bộ nhớ lưu session */
        //     server->sess = (http_session_t *)realloc(server->sess, sizeof(void (*)) * (sess_count + 2));
        // }
        // if(server->sess) {
        //     server->sess[sess_count] = new_sess;
        //     server->sess[sess_count + 1] = 0;
        //     return new_sess;
        // }
        return new_sess;
    }
    return 0;
}
static void http_server_session_free(http_session_t *sess) {
    HTTP_LOGD(__FILE__":%d HTTP sess free: sess=0x%08X pcb=0x%08X\r\n", __LINE__, sess, sess->client_tcp);
    http_server_t *server = sess->server;
    uint8_t sess_count = 0;
    
    if(sess) {
        if(sess->response.fd > 0) {
            sess->response.content_pos = 0;
            sess->response.content_len = 0;
            if(sess->response.content) {
                free(sess->response.content);
            }
            fclose(sess->response.fd);
        }
        if(sess->ws_parser) {
            free(sess->ws_parser);
        }
        if(sess->request.websocket.key) {
            free(sess->request.websocket.key);
        }
        if(sess->request.form_data.boundary) {
            free(sess->request.form_data.boundary);
        }
        if(sess->response.content) {
            free(sess->response.content);
        }
        free(sess);
    }
    // memset(sess, 0, sizeof(http_session_t));
    // if(server->sess != 0) {
    //     /* Đếm số lượng session hiện tại */
    //     for(sess_count = 0; server->sess[sess_count] != 0; sess_count++) {
    //     }
    //     if(sess_count == 1) {
    //         free(server->sess[0]);
    //         free(server->sess);
    //         server->sess = 0;
    //     }
    //     else if(sess_count > 1) {
    //         /* Tìm vị trí của session cần xóa, free nó và thay thế nó bằng session ở cuối cùng, realloc lại list */
    //         for(uint8_t idx_sess = 0; server->sess[idx_sess] != 0; idx_sess++) {
    //             if(sess == server->sess[idx_sess]) {
    //                 free(sess);
    //                 server->sess[idx_sess] = server->sess[sess_count - 1];
    //                 server->sess = (http_session_t *)realloc(server->sess, sizeof(void (*)) * (sess_count));
    //                 server->sess[sess_count - 1] = 0;
    //             }
    //         }
    //     }
    // }
}
void http_server_register_uri(http_server_t *server, http_uri_t *resp) {
    if(resp == 0) return;
    if(server->route.count == 0) {
        server->route.index = (http_uri_t **)malloc(sizeof(void *));
        server->route.index[server->route.count] = resp;
        server->route.count = 1;
    }
    else {
        server->route.count++;
        server->route.index = (http_uri_t **)realloc(server->route.index, server->route.count * sizeof(void *));
        server->route.index[server->route.count - 1] = resp;
    }
}
void http_server_unregister_uri(http_server_t *server, http_uri_t *resp) {
    if(server->route.count == 0) {
        return;
    }
    if(server->route.count == 1) {
        server->route.count = 0;
        free(server->route.index);
        return;
    }
    for(uint8_t idx_resp = 0; idx_resp < server->route.count; idx_resp++) {
        if(server->route.index[idx_resp] == resp) {
            server->route.count--;
            server->route.index[idx_resp] = server->route.index[server->route.count];
            server->route.index = (http_uri_t **)realloc(server->route.index, server->route.count * sizeof(void *));
        }
    }
}