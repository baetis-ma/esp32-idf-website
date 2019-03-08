#define EXAMPLE_WIFI_SSID "troutstream"
#define EXAMPLE_WIFI_PASS "password"
#define PORT 80

void wait_for_ip();
void initialize_wifi(void);
void tcp_server_task(void *pvParameters);


