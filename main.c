#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <string.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

const int led_gpio = 2;

#define MOTION_SENSOR_GPIO 12

void led_write(bool on) {
        gpio_write(led_gpio, on ? 0 : 1);
}

void identify_task(void *_args) {
  // We identify the `Motion Sensor` by Flashing it's LED.
  for (int i=0; i<3; i++) {
          for (int j=0; j<2; j++) {
                  led_write(true);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
                  led_write(false);
                  vTaskDelay(100 / portTICK_PERIOD_MS);
          }

          vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(false);

  vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
    printf("identify\n\n");
    xTaskCreate(identify_task, "identify", 128, NULL, 2, NULL);
}

homekit_characteristic_t motion_detected  = HOMEKIT_CHARACTERISTIC_(MOTION_DETECTED, 0);

void motion_sensor_callback(uint8_t gpio) {


    if (gpio == MOTION_SENSOR_GPIO){
        int new = 0;
        new = gpio_read(MOTION_SENSOR_GPIO);
        motion_detected.value = HOMEKIT_BOOL(new);
        homekit_characteristic_notify(&motion_detected, HOMEKIT_BOOL(new));
        if (new == 1) {
		printf("Motion Detected on %d\n", gpio);
        } else {
        printf("Motion Stopped on %d\n", gpio);

//        vTaskResume( https_post_wolfssl_task_handle );
	}
    }
    else {
        printf("Interrupt on %d", gpio);
    }

}

void gpio_init() {

    gpio_enable(MOTION_SENSOR_GPIO, GPIO_INPUT);
    gpio_set_pullup(MOTION_SENSOR_GPIO, false, false);
    gpio_set_interrupt(MOTION_SENSOR_GPIO, GPIO_INTTYPE_EDGE_ANY, motion_sensor_callback);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Motion Sensor");

homekit_accessory_t *accessories[] = {
        HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
                HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
                        &name,
                        HOMEKIT_CHARACTERISTIC(MANUFACTURER, "StudioPieters®"),
                        HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "037A2BABF19D"),
                        HOMEKIT_CHARACTERISTIC(MODEL, "Motion Sensor"),
                        HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1.6"),
                        HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
                        NULL
                }),
                HOMEKIT_SERVICE(MOTION_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]){
                        HOMEKIT_CHARACTERISTIC(NAME, "Motion Sensor"),
                        &motion_detected,
                        NULL
                }),
                NULL
        }),
        NULL
};

homekit_server_config_t config = {
        .accessories = accessories,
        .password = "558-98-144",
        .setupId="9SW7",
};

void on_wifi_ready() {
}

void create_accessory_name() {
        uint8_t macaddr[6];
        sdk_wifi_get_macaddr(STATION_IF, macaddr);

        int name_len = snprintf(NULL, 0, "Motion Sensor-%02X%02X%02X",
                                macaddr[3], macaddr[4], macaddr[5]);
        char *name_value = malloc(name_len+1);
        snprintf(name_value, name_len+1, "Motion Sensor-%02X%02X%02X",
                 macaddr[3], macaddr[4], macaddr[5]);

        name.value = HOMEKIT_STRING(name_value);
}

void user_init(void) {
        uart_set_baud(0, 115200);
          gpio_init();

        create_accessory_name();

        wifi_config_init("Motion Sensor", NULL, on_wifi_ready);
        gpio_init();


        homekit_server_init(&config);


}
