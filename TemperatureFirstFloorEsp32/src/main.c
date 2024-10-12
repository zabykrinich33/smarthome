#include <stdio.h> //for basic printf commands
#include <string.h> //for handling strings
#include "freertos/FreeRTOS.h" //for delay,mutexs,semphrs rtos operations
#include "freertos/task.h"
#include "esp_system.h" //esp_init funtions esp_err_t 
#include "esp_wifi.h" //esp_wifi_init functions and wifi operations
#include "esp_log.h" //for showing logs
#include "esp_event.h" //for wifi event
#include "nvs_flash.h" //non volatile storage
#include "lwip/err.h" //light weight ip packets error handling
#include "lwip/sys.h" //system applications for light weight ip apps
#include "DHT22.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "sdkconfig.h"
#include <esp_http_client.h>
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include <time.h> 

const char *ssid = "HOME_Z";
const char *pass = "Mm357911Tt";

//const char *ssid = "Bahnhof-2.4G-HE4LXX";
//const char *pass = "MJE39JJA";
int retry_num=0;

#define INFLUXDB_HOST "192.168.1.5"
//#define INFLUXDB_HOST "192.168.1.2"
#define INFLUXDB_PORT "8086"
#define INFLUXDB_USERNAME "home"
#define INFLUXDB_PASSWORD "12345"
#define INFLUXDB_DATABASE "home_temperature"

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
if(event_id == WIFI_EVENT_STA_START)
{
  printf("WIFI CONNECTING....\n");
}
else if (event_id == WIFI_EVENT_STA_CONNECTED)
{
  printf("WiFi CONNECTED\n");
}
else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
{
  printf("WiFi lost connection\n");
  if(retry_num<100){esp_wifi_connect();retry_num++;printf("Retrying to Connect...\n");}
  else abort();
}
else if (event_id == IP_EVENT_STA_GOT_IP)
{
  printf("Wifi got IP...\n\n");
}
}

void wifi_connection()
{
     //                          s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_netif_init();
    esp_event_loop_create_default();     // event loop                    s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                      s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //     
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
            
           }
    
        };
    strcpy((char*)wifi_configuration.sta.ssid, ssid);
    strcpy((char*)wifi_configuration.sta.password, pass);    
    //esp_log_write(ESP_LOG_INFO, "Kconfig", "SSID=%s, PASS=%s", ssid, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
    printf( "wifi_init_softap finished. SSID:%s  password:%s",ssid,pass);
    
}



esp_http_client_handle_t influxdb_setup(){
    esp_http_client_config_t config = {
       .url = "http://" INFLUXDB_HOST ":" INFLUXDB_PORT "/write?db=" INFLUXDB_DATABASE,
       .auth_type = HTTP_AUTH_TYPE_BASIC,
       .username = INFLUXDB_USERNAME,
       .password = INFLUXDB_PASSWORD,
   };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    return client;
}

void send_influxdb_data(esp_http_client_handle_t client, float temp, float hum, const char* name) {
    char data[128];
    snprintf(data, sizeof(data), "%s temperature=%f,humidity=%f", name, temp, hum);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, data, strlen(data));
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        printf("%s Data sent to InfluxDB successfully", name);
    }
    else
    {
        printf("Error sending %s data to InfluxDB: %s", name, esp_err_to_name(err));
    }
}

void DHT_read_task(void *pvParameter)
{

  esp_http_client_handle_t influx_client = influxdb_setup();

	while(1) {

    	setDHTgpio(GPIO_NUM_4);

	    printf("DHT bathroom Sensor Readings\n" );
	    int retBathroom = readDHT();
		
	    errorHandler(retBathroom);
      
      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "bathroom");

	    printf("Humidity bathroom %.2f %%\n", getHumidity());
	    printf("Temperature bathroom %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );

    	setDHTgpio(GPIO_NUM_5);

	    printf("DHT main Sensor Readings\n" );
	    int retMain = readDHT();
		
	    errorHandler(retMain);

      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "main");

	    printf("Humidity main %.2f %%\n", getHumidity());
	    printf("Temperature main %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );
    }
}

void app_main(void)
{
nvs_flash_init();
vTaskDelay(2000 / portTICK_PERIOD_MS );
wifi_connection();
vTaskDelay(2000 / portTICK_PERIOD_MS );

esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
esp_netif_sntp_init(&config);
if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
    printf("Failed to update system time within 10s timeout");
}

xTaskCreate(&DHT_read_task, "DHT_read_task", 4096, NULL, 5, NULL );
//xTaskCreate(&DHT_el_task, "DHT_el_task", 2048, NULL, 5, NULL );
//xTaskCreate(&DHT_porn_task, "DHT_porn_task", 2048, NULL, 5, NULL );
//xTaskCreate(&DHT_cold_task, "DHT_cold_task", 2048, NULL, 5, NULL );


}