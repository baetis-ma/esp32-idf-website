//this lower level source file contains following functions:
//  1) initialize_wifi - sets up connection to locl wifi network
//  2) event_handler   - sets up wifi connection event handler
//  3) wait_for_ip     - waits for wifi connection
//  4) tcp_server_task - sets up loop to listen for and handle http requests from client
//                       current fonfigured for three types of payload headers
//                       -- GET /index.html or GET/ returns embedded index.html file
//                       -- GET /(not index.html or blank) returns 404 page
//                       -- Get /GetData runs function in top level source to return something like data
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "sdkconfig.h"

//local wifi credentials and port number to listen on
#define EXAMPLE_WIFI_SSID "troutstream"
#define EXAMPLE_WIFI_PASS "password"
#define PORT 80

//need to forward define funtion in top level source
void GetData(int sock, char *url_responce, int );

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;
const int IPV4_GOTIP_BIT = BIT0;
static const char *TAG = "webpage ";
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void initialize_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void wait_for_ip()
{
    uint32_t bits = IPV4_GOTIP_BIT;
    //ESP_LOGI(TAG, "Waiting for AP connection...");
    xEventGroupWaitBits(wifi_event_group, bits, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");
}


void tcp_server_task(void *pvParameters)
{
    int addr_family;
    int ip_protocol;
    char addr_str[20];
    char rx_buffer[500];

    while (1) {

        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        listen(listen_sock, 1);

        struct sockaddr_in sourceAddr;
        uint addrLen = sizeof(sourceAddr);
        while (1) {
            int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
            if (sock < 0) { ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno); break; }
            ESP_LOGI(TAG, "Socket accepted");

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            rx_buffer[len] = 0; // Null-terminate whatever we received so it acts like a string

            // Error occured during receiving
            if (len <= 0) { ESP_LOGE(TAG, "recv failed: errno %d", errno); break; }
            // Data received
            else {
                //prints client source ip and http request packet to esp monitor
                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                ESP_LOGI("---  ", "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI("", "HTTP REQUEST Packet\n%s", rx_buffer);

                //parse request type and arg 
                char http_type[8];
                char url_resource[64];
                char url_name[16];
                char temp_str[64]; //setup temp_str with reource name only

                sscanf(rx_buffer, "%s /%s", http_type, temp_str);
                if ( strstr(temp_str, "?") ) {
                    size_t quesmark = (int) (strchr(temp_str, '?') - temp_str);
                    strcpy (url_resource, temp_str + quesmark + 1 );
                    strcpy (url_name, temp_str );
                    url_name[quesmark] = '\0';
                } else {
                    strcpy (url_name, temp_str);
                    url_resource[0] = '\0';
                }
                //ESP_LOGI("", "request type %s|%s|%s", http_type, url_name, url_resource);

                //reads file directly from read only data space (saving ram buffer space)
                //and breaks up file to fit ip buffer size 
                if (strcmp("index.html", url_name) ==0 || strcmp("HTTP/1.1",url_name) ==0 ){
                    extern const char index_start[] asm("_binary_index_html_start");
                    extern const char index_end[] asm("_binary_index_html_end");
                    int pkt_buf_size = 1500;
                    int pkt_end = pkt_buf_size;
                    int ro_len =  strlen(index_start) - strlen(index_end);
                    for( int pkt_ptr = 0; pkt_ptr < ro_len; pkt_ptr = pkt_ptr + pkt_buf_size){
                        if ((ro_len - pkt_ptr) < pkt_buf_size) pkt_end = ro_len - pkt_ptr;
                            ESP_LOGI(TAG, "pkt_ptr %d pkt_end %d", pkt_ptr,pkt_end );
                            send(sock, index_start + pkt_ptr, pkt_end, 0);
                        }
                } else 
                if (strcmp("GET", http_type) ==0 && strcmp("getData",url_name) ==0 ) {
                    GetData(sock, url_resource, strlen(url_resource));
                } else { 
                    char temp404[] = "<html><h1>Not Found 404</html>";
                    send(sock, temp404, sizeof(temp404), 0);
                    ESP_LOGI("", "404");
                }
                vTaskDelay(10/ portTICK_RATE_MS); //waits for buffer to purge
                shutdown(sock, 0);
                close(sock);
            }
        }
    }
    vTaskDelete(NULL);
}
