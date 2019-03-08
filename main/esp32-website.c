#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
//#include "esp_wifi.h"
#include "esp_event_loop.h"
//#include "esp_log.h"
#include "nvs_flash.h"

#include "./wifi-init.h"
//#include "lwip/err.h"
#include "lwip/sockets.h"
//#include "lwip/sys.h"
//#include <lwip/netdb.h>

//#include "sdkconfig.h"
#define CPU_FREQ    240

uint32_t ccount_stamp;
uint32_t IRAM_ATTR cycles(int reset)
{
    uint32_t ccount;
    __asm__ __volatile__ ( "rsr     %0, ccount" : "=a" (ccount) );

    if (reset == 1){ ccount_stamp = ccount; ccount = 0; }
    else { ccount = ccount - ccount_stamp; }
    ccount = ccount / CPU_FREQ;
    printf(" timer =  %10.3f ms\n", (float) ccount/1000);

    return ccount;
}

void GetData( int sock, char *url_resource, int length ) {
  char temp[100] = "<!DOCTYPE HTML>\n<html><h1>getData<h1></body></html>";
  url_resource[length] = '\0';
  send(sock, temp, sizeof(temp), 0);
}

void app_main()
{
    nvs_flash_init();
    initialize_wifi();
    wait_for_ip();
 
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);

}
