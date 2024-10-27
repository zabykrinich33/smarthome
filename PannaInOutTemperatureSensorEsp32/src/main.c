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
//#include "DHT22.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "sdkconfig.h"
#include <esp_http_client.h>
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include <time.h> 

/*#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"*/

#include "ds18b20.h"

const char *ssid = "HOME_Z";
const char *pass = "Mm357911Tt";

//const char *ssid = "Bahnhof-2.4G-HE4LXX";
//const char *pass = "MJE39JJA";
int retry_num=0;

#define GPIO_DS18B20_0       (/*CONFIG_ONE_WIRE_GPIO*/GPIO_NUM_4)
#define MAX_DEVICES          (8)
#define DS18B20_RESOLUTION   (DS18B20_RESOLUTION_12_BIT)
#define SAMPLE_PERIOD        (1000)   // milliseconds

#define INFLUXDB_HOST "192.168.1.5"
//#define INFLUXDB_HOST "192.168.1.2"
#define INFLUXDB_PORT "8086"
#define INFLUXDB_USERNAME "home"
#define INFLUXDB_PASSWORD "12345"
#define INFLUXDB_DATABASE "panna_temp_in_out"

// Temp Sensors are on GPIO26
#define TEMP_BUS 4
#define LED 2
#define HIGH 1
#define LOW 0
#define digitalWrite gpio_set_level

DeviceAddress tempSensors[3];


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
  if(retry_num<20){esp_wifi_connect();retry_num++;printf("Retrying to Connect...\n");}
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

void send_influxdb_data(esp_http_client_handle_t client, float temp, const char* name) {
    char data[128];
    snprintf(data, sizeof(data), "%s temperature=%f", name, temp);

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


/*  esp_http_client_handle_t influx_client = influxdb_setup();

	while(1) {

    	setDHTgpio(GPIO_NUM_4);

	    printf("DHT Panna Sensor Readings\n" );
	    int retPanna = readDHT();
		
	    errorHandler(retPanna);
      
      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "panna");

	    printf("Humidity panna %.2f %%\n", getHumidity());
	    printf("Temperature panna %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );

    	setDHTgpio(GPIO_NUM_5);

	    printf("DHT el Sensor Readings\n" );
	    int retEl = readDHT();
		
	    errorHandler(retEl);

      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "el");

	    printf("Humidity el %.2f %%\n", getHumidity());
	    printf("Temperature el %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );

    	setDHTgpio(GPIO_NUM_6);

	    printf("DHT porn Sensor Readings\n" );
	    int retPorn = readDHT();
		
	    errorHandler(retPorn);

      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "porn");

	    printf("Humidity porn %.2f %%\n", getHumidity());
	    printf("Temperature porn %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );

    	setDHTgpio(GPIO_NUM_7);

	    printf("DHT cold Sensor Readings\n" );
	    int retCold = readDHT();
		
	    errorHandler(retCold);

      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "cold");

	    printf("Humidity cold %.2f %%\n", getHumidity());
	    printf("Temperature cold %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );
    }*/
}


void getTempAddresses(DeviceAddress *tempSensorAddresses) {
	unsigned int numberFound = 0;
	reset_search();
	// search for 3 addresses on the oneWire protocol
	while (search(tempSensorAddresses[numberFound],true)) {
		numberFound++;
		if (numberFound == 3) break;
	}
	// if 3 addresses aren't found then flash the LED rapidly
	while (numberFound != 3) {
		numberFound = 0;
		digitalWrite(LED, HIGH);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		digitalWrite(LED, LOW);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		// search in the loop for the temp sensors as they may hook them up
		reset_search();
		while (search(tempSensorAddresses[numberFound],true)) {
			numberFound++;
			if (numberFound == 3) break;
		}
	}
	return;
}


void DS18B20_read_task(void *pvParameter)
{

    esp_http_client_handle_t influx_client = influxdb_setup();

    ds18b20_init(TEMP_BUS);
	getTempAddresses(tempSensors);
	ds18b20_setResolution(tempSensors,3,10);

	printf("Address 0: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n", tempSensors[0][0],tempSensors[0][1],tempSensors[0][2],tempSensors[0][3],tempSensors[0][4],tempSensors[0][5],tempSensors[0][6],tempSensors[0][7]);
	printf("Address 1: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n", tempSensors[1][0],tempSensors[1][1],tempSensors[1][2],tempSensors[1][3],tempSensors[1][4],tempSensors[1][5],tempSensors[1][6],tempSensors[1][7]);
	printf("Address 2: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n", tempSensors[2][0],tempSensors[2][1],tempSensors[2][2],tempSensors[2][3],tempSensors[2][4],tempSensors[2][5],tempSensors[2][6],tempSensors[2][7]);


	while (1) {
		ds18b20_requestTemperatures();
		float temp1 = ds18b20_getTempF((DeviceAddress *)tempSensors[0]);
		float temp2 = ds18b20_getTempF((DeviceAddress *)tempSensors[1]);
        float temp3 = ds18b20_getTempF((DeviceAddress *)tempSensors[2]);
		float temp4 = ds18b20_getTempC((DeviceAddress *)tempSensors[0]);
		float temp5 = ds18b20_getTempC((DeviceAddress *)tempSensors[1]);
        float temp6 = ds18b20_getTempC((DeviceAddress *)tempSensors[2]);

		printf("Temperatures: %0.1fF %0.1fF %0.1fF\n", temp1,temp2,temp3);
		printf("Temperatures: %0.1fC %0.1fC %0.1fC\n", temp4,temp5,temp6);

        if (temp4 > -1 && temp4 < 60) {
            send_influxdb_data(influx_client, temp4, "panna_temp_out");
        }
        if (temp5 > -1 && temp5 < 60) {
            send_influxdb_data(influx_client, temp5, "panna_temp_in");
        }
        if (temp6 > -1 && temp6 < 60) {
            send_influxdb_data(influx_client, temp6, "temp_street");
        }
        
		float cTemp = ds18b20_get_temp();
		printf("Temperature: %0.1fC\n", cTemp);
		vTaskDelay(60000 / portTICK_PERIOD_MS);
	}




  /*esp_http_client_handle_t influx_client = influxdb_setup();

    // Stable readings require a brief period before communication
    vTaskDelay(2000.0 / portTICK_PERIOD_MS);

    // Create a 1-Wire bus, using the RMT timeslot driver
    OneWireBus * owb;
    owb_rmt_driver_info rmt_driver_info;
    owb = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    // Find all connected devices
    printf("Find devices:\n");
    OneWireBus_ROMCode device_rom_codes[MAX_DEVICES] = {0};
    int num_devices = 0;
    OneWireBus_SearchState search_state = {0};
    bool found = false;
    owb_search_first(owb, &search_state, &found);
    while (found)
    {
        char rom_code_s[17];
        owb_string_from_rom_code(search_state.rom_code, rom_code_s, sizeof(rom_code_s));
        printf("  %d : %s\n", num_devices, rom_code_s);
        device_rom_codes[num_devices] = search_state.rom_code;
        ++num_devices;
        owb_search_next(owb, &search_state, &found);
    }
    printf("Found %d device%s\n", num_devices, num_devices == 1 ? "" : "s");

    // In this example, if a single device is present, then the ROM code is probably
    // not very interesting, so just print it out. If there are multiple devices,
    // then it may be useful to check that a specific device is present.

    if (num_devices == 1)
    {
        // For a single device only:
        OneWireBus_ROMCode rom_code;
        owb_status status = owb_read_rom(owb, &rom_code);
        if (status == OWB_STATUS_OK)
        {
            char rom_code_s[OWB_ROM_CODE_STRING_LENGTH];
            owb_string_from_rom_code(rom_code, rom_code_s, sizeof(rom_code_s));
            printf("Single device %s present\n", rom_code_s);
        }
        else
        {
            printf("An error occurred reading ROM code: %d", status);
        }
    }
    else
    {
        // Search for a known ROM code (LSB first):
        // For example: 0x1502162ca5b2ee28
        OneWireBus_ROMCode known_device = {
            .fields.family = { 0x28 },
            .fields.serial_number = { 0xee, 0xb2, 0xa5, 0x2c, 0x16, 0x02 },
            .fields.crc = { 0x15 },
        };
        char rom_code_s[OWB_ROM_CODE_STRING_LENGTH];
        owb_string_from_rom_code(known_device, rom_code_s, sizeof(rom_code_s));
        bool is_present = false;

        owb_status search_status = owb_verify_rom(owb, known_device, &is_present);
        if (search_status == OWB_STATUS_OK)
        {
            printf("Device %s is %s\n", rom_code_s, is_present ? "present" : "not present");
        }
        else
        {
            printf("An error occurred searching for known device: %d", search_status);
        }
    }

    // Create DS18B20 devices on the 1-Wire bus
    DS18B20_Info * devices[MAX_DEVICES] = {0};
    for (int i = 0; i < num_devices; ++i)
    {
        DS18B20_Info * ds18b20_info = ds18b20_malloc();  // heap allocation
        devices[i] = ds18b20_info;

        if (num_devices == 1)
        {
            printf("Single device optimisations enabled\n");
            ds18b20_init_solo(ds18b20_info, owb);          // only one device on bus
        }
        else
        {
            ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); // associate with bus and device
        }
        ds18b20_use_crc(ds18b20_info, true);           // enable CRC check on all reads
        ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);
    }

    // Read temperatures from all sensors sequentially
    while (1)
    {
        printf("\nTemperature readings (degrees C):\n");
        for (int i = 0; i < num_devices; ++i)
        {
            float temp = 0;
            DS18B20_ERROR error = ds18b20_read_temp(devices[i], &temp);
            printf("  %d: %.3f\n", i, temp);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }*/

    // Check for parasitic-powered devices
   /* bool parasitic_power = false;
    ds18b20_check_for_parasite_power(owb, &parasitic_power);
    if (parasitic_power) {
        printf("Parasitic-powered devices detected");
    }

    // In parasitic-power mode, devices cannot indicate when conversions are complete,
    // so waiting for a temperature conversion must be done by waiting a prescribed duration
    owb_use_parasitic_power(owb, parasitic_power);

#ifdef CONFIG_ENABLE_STRONG_PULLUP_GPIO
    // An external pull-up circuit is used to supply extra current to OneWireBus devices
    // during temperature conversions.
    owb_use_strong_pullup_gpio(owb, CONFIG_STRONG_PULLUP_GPIO);
#endif

    // Read temperatures more efficiently by starting conversions on all devices at the same time
    int errors_count[MAX_DEVICES] = {0};
    int sample_count = 0;
    if (num_devices > 0)
    {
        TickType_t last_wake_time = xTaskGetTickCount();

        while (1)
        {
            ds18b20_convert_all(owb);

            // In this application all devices use the same resolution,
            // so use the first device to determine the delay
            ds18b20_wait_for_conversion(devices[0]);

            // Read the results immediately after conversion otherwise it may fail
            // (using printf before reading may take too long)
            float readings[MAX_DEVICES] = { 0 };
            DS18B20_ERROR errors[MAX_DEVICES] = { 0 };

            for (int i = 0; i < num_devices; ++i)
            {
                errors[i] = ds18b20_read_temp(devices[i], &readings[i]);
            }

            // Print results in a separate loop, after all have been read
            printf("\nTemperature readings (degrees C): sample %d\n", ++sample_count);
            for (int i = 0; i < num_devices; ++i)
            {
                if (errors[i] != DS18B20_OK)
                {
                    ++errors_count[i];
                }

                printf("  %d: %.1f    %d errors\n", i, readings[i], errors_count[i]);
            }

            vTaskDelayUntil(&last_wake_time, SAMPLE_PERIOD / portTICK_PERIOD_MS);
        }
    }
    else
    {
        printf("\nNo DS18B20 devices detected!\n");
    }*/

	//while(1) {

    	/*setDHTgpio(GPIO_NUM_4);

	    printf("DHT Panna Sensor Readings\n" );
	    int retPanna = readDHT();
		
	    errorHandler(retPanna);
      
      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "panna");

	    printf("Humidity panna %.2f %%\n", getHumidity());
	    printf("Temperature panna %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );

    	setDHTgpio(GPIO_NUM_5);

	    printf("DHT el Sensor Readings\n" );
	    int retEl = readDHT();
		
	    errorHandler(retEl);

      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "el");

	    printf("Humidity el %.2f %%\n", getHumidity());
	    printf("Temperature el %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );

    	setDHTgpio(GPIO_NUM_6);

	    printf("DHT porn Sensor Readings\n" );
	    int retPorn = readDHT();
		
	    errorHandler(retPorn);

      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "porn");

	    printf("Humidity porn %.2f %%\n", getHumidity());
	    printf("Temperature porn %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );

    	setDHTgpio(GPIO_NUM_7);

	    printf("DHT cold Sensor Readings\n" );
	    int retCold = readDHT();
		
	    errorHandler(retCold);

      send_influxdb_data(influx_client, getTemperature(), getHumidity(), "cold");

	    printf("Humidity cold %.2f %%\n", getHumidity());
	    printf("Temperature cold %.2f degC\n\n", getTemperature());
		
	    vTaskDelay(2000 / portTICK_PERIOD_MS );*/
    //}
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

xTaskCreate(&DS18B20_read_task, "DS18B20_read_task", 4096, NULL, 5, NULL );
//xTaskCreate(&DHT_read_task, "DHT_read_task", 4096, NULL, 5, NULL );
//xTaskCreate(&DHT_el_task, "DHT_el_task", 2048, NULL, 5, NULL );
//xTaskCreate(&DHT_porn_task, "DHT_porn_task", 2048, NULL, 5, NULL );
//xTaskCreate(&DHT_cold_task, "DHT_cold_task", 2048, NULL, 5, NULL );


}