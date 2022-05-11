#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#define EXAMPLE_WIFI_SSID "troutstream"
#define EXAMPLE_WIFI_PASS "password"
#define PORT 80
char glob_ipadr[25];
char glob_ipadrc[25];
void wait_for_ip();
void initialize_wifi(void);
void tcp_server_task(void *pvParameters);

void GetData( int sock, char *url_resource, int length ) {
  char temp[1000];
  sprintf(temp,"\
     <!DOCTYPE HTML>\                  
     <html>\                  
     <head>\                  
     <title> Hello World!</title>\                  
     <style>\                  
       h1 { text-align:center; color:blue; }\                  
       h2 { text-align:center; color:red; }\                  
       h5 { text-align:center; color:green; }\                  
     </style>\                  
     </head>\                  
     <body>\                  
     <h1>getData() subroutine<h1>\                  
     <h2>Thanks for sending me --> %s<h2>\
     <h5>my ip is %s<h5>\
     <h5>your ip is %s<h5></body></html>\
     </body>\                  
     </html>\n", url_resource, glob_ipadr, glob_ipadrc);

  temp[strlen(temp)] = '\0';
  send(sock, temp, strlen(temp), 0);
}

void app_main()
{
    nvs_flash_init();
    initialize_wifi();
    wait_for_ip();
 
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);

}
