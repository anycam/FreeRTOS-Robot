/* 
Codigo para robot controlado por medio de TCP server con FreeRTOS.
Equipo 2
CAMARENA GARCIA ANDRE
FLORES LARA SERGIO
MORENO HERNANDEZ JOSE ANTONIO
CRUZ TORRES LUIS FERNANDO

F.E.S. ARAGON - UNAM
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//============ BATTERY ==========================================
#include <stdlib.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

//------------- GPIO --------------------------------------------
#include <stdio.h>
#include "driver/gpio.h"
#include "sdkconfig.h"
#define BLINK_GPIO_a 17
#define BLINK_GPIO_b 21
#define BLINK_GPIO_c 15
#define BLINK_GPIO_d 12
//================== BATTERY ====================================
#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling
#define BLINK_GPIO_l 27             //LED indicator
//----------------------------------------------------------------
#define PORT                        CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT

static const char *TAG = "CaminanteTest";

//==================== BATTERY =================================
static esp_adc_cal_characteristics_t *adc_chars;
#if CONFIG_IDF_TARGET_ESP32
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#elif CONFIG_IDF_TARGET_ESP32S2
static const adc_channel_t channel = ADC_CHANNEL_6;     // GPIO7 if ADC1, GPIO17 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_13;
#endif
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

//====================Battery LED TASK=======================
static void blink_task_L(int n)
{
    int i;

    gpio_pad_select_gpio(BLINK_GPIO_l);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO_l, GPIO_MODE_OUTPUT);
    for(i=0; i<n; i++) {
    /* LED ON */
    gpio_set_level(BLINK_GPIO_l, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO_l, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    }
}

static void blink_task_LB(int n)
{
    int i;

    gpio_pad_select_gpio(BLINK_GPIO_l);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO_l, GPIO_MODE_OUTPUT);
    for(i=0; i<n; i++) {
    /* LED ON */
    gpio_set_level(BLINK_GPIO_l, 1);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO_l, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void check_efuse(void)
{
#if CONFIG_IDF_TARGET_ESP32
    //Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
#elif CONFIG_IDF_TARGET_ESP32S2
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("Cannot retrieve eFuse Two Point calibration values. Default calibration values will be used.\n");
    }
#else
#error "This example is configured for ESP32/ESP32S2."
#endif
}


static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

//------------- GPIO --------------------------------------------
static void blink_task_A(int n)
{
    int i;

    gpio_pad_select_gpio(BLINK_GPIO_a);
    gpio_pad_select_gpio(BLINK_GPIO_b);
    gpio_pad_select_gpio(BLINK_GPIO_c);
    gpio_pad_select_gpio(BLINK_GPIO_d);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO_a, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_b, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_c, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_d, GPIO_MODE_OUTPUT);
    for(i=0; i<n; i++) {
    /* Motor Adelante */
    gpio_set_level(BLINK_GPIO_a, 0);
    gpio_set_level(BLINK_GPIO_b, 1);
    gpio_set_level(BLINK_GPIO_c, 0);
    gpio_set_level(BLINK_GPIO_d, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    /* Motor stop */
    gpio_set_level(BLINK_GPIO_a, 0);
    gpio_set_level(BLINK_GPIO_b, 0);
    gpio_set_level(BLINK_GPIO_c, 0);
    gpio_set_level(BLINK_GPIO_d, 0);
}

static void blink_task_B(int n)
{
    int i;

    gpio_pad_select_gpio(BLINK_GPIO_a);
    gpio_pad_select_gpio(BLINK_GPIO_b);
    gpio_pad_select_gpio(BLINK_GPIO_c);
    gpio_pad_select_gpio(BLINK_GPIO_d);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO_a, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_b, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_c, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_d, GPIO_MODE_OUTPUT);
    for(i=0; i<n; i++) {
    /* Motor Atras */
    gpio_set_level(BLINK_GPIO_a, 1);
    gpio_set_level(BLINK_GPIO_b, 0);
    gpio_set_level(BLINK_GPIO_c, 1);
    gpio_set_level(BLINK_GPIO_d, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    /* Motor stop */
    gpio_set_level(BLINK_GPIO_a, 0);
    gpio_set_level(BLINK_GPIO_b, 0);
    gpio_set_level(BLINK_GPIO_c, 0);
    gpio_set_level(BLINK_GPIO_d, 0);
}
static void blink_task_C(int n)
{
    int i;

    gpio_pad_select_gpio(BLINK_GPIO_a);
    gpio_pad_select_gpio(BLINK_GPIO_b);
    gpio_pad_select_gpio(BLINK_GPIO_c);
    gpio_pad_select_gpio(BLINK_GPIO_d);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO_a, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_b, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_c, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_d, GPIO_MODE_OUTPUT);
    for(i=0; i<n; i++) {
    /* Giro uno */
    gpio_set_level(BLINK_GPIO_a, 0);
    gpio_set_level(BLINK_GPIO_b, 1);
    gpio_set_level(BLINK_GPIO_c, 0);
    gpio_set_level(BLINK_GPIO_d, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    /* Motor stop */
    gpio_set_level(BLINK_GPIO_a, 0);
    gpio_set_level(BLINK_GPIO_b, 0);
    gpio_set_level(BLINK_GPIO_c, 0);
    gpio_set_level(BLINK_GPIO_d, 0);
}

static void blink_task_D(int n)
{
    int i;
    gpio_pad_select_gpio(BLINK_GPIO_a);
    gpio_pad_select_gpio(BLINK_GPIO_b);
    gpio_pad_select_gpio(BLINK_GPIO_c);
    gpio_pad_select_gpio(BLINK_GPIO_d);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO_a, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_b, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_c, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLINK_GPIO_d, GPIO_MODE_OUTPUT);
    for(i=0; i<n; i++) {
    /* Giro Dos */
    gpio_set_level(BLINK_GPIO_a, 0);
    gpio_set_level(BLINK_GPIO_b, 0);
    gpio_set_level(BLINK_GPIO_c, 0);
    gpio_set_level(BLINK_GPIO_d, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    /* Motor stop */
    gpio_set_level(BLINK_GPIO_a, 0);
    gpio_set_level(BLINK_GPIO_b, 0);
    gpio_set_level(BLINK_GPIO_c, 0);
    gpio_set_level(BLINK_GPIO_d, 0);
}
//---------------------------------------------------------------

static void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            //--------------------------Recepcion de datos-------------------------
            char *token = strtok(rx_buffer,",");
            char delimitador[] = ",";
            char instruccion[10], dato[10];
            strcpy(instruccion,token);
            token = strtok(NULL, delimitador);
            strcpy(dato,token);
            //---------------------------------------------------------------------
            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation.
            int to_write = len;
            while (to_write > 0) {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }
                //------------- GPIO --------------------------------------------
                else {
                    switch(instruccion[0]) {
                        case 'a':
                        ESP_LOGI(TAG, "Camina hacia Adelante");
                        blink_task_A(atoi(dato));
                        break;
                        case 'r':
                        ESP_LOGI(TAG, "Camina de Reversa");
                        blink_task_B(atoi(dato));
                        break;
                        case 'i':
                        ESP_LOGI(TAG, "Gira a la Izquierda");
                        blink_task_C(atoi(dato));
                        break;
                        case 'd':
                        ESP_LOGI(TAG, "Gira a la Derecha");
                        blink_task_D(atoi(dato));
                        break;
                        default:
                        ESP_LOGI(TAG, "Comando no reconocido, Default %s",dato);
                        break;
                        }
                }
                // ----------------------------------------------------------

                to_write -= written;
            }
        }
    } while (len > 0);
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#ifdef CONFIG_EXAMPLE_IPV6
    else if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#ifdef CONFIG_EXAMPLE_IPV6
        else if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

//=========================BATTERY TASK!!!============================
static void battery_task(void *pvParameters)
{
    //Check if Two Point or Vref are burned into eFuse
    check_efuse();
    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(width);
        adc1_config_channel_atten(channel, atten);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
    //Continuously sample ADC1
    while (1) {
        uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            if (unit == ADC_UNIT_1) {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            } else {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, width, &raw);
                adc_reading += raw;
            }
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
        if(adc_reading<3000){
            blink_task_L(1);
        }
        else{

            blink_task_LB(1);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
    //===========================END OF BATTERY TASK KILLING PROCESS===========================
}

void app_main(void)
{

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

#ifdef CONFIG_EXAMPLE_IPV4
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    xTaskCreate(battery_task, "battery_task", 4096, NULL, 4, NULL);
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, NULL);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    xTaskCreate(battery_task, "battery_task", 4096, NULL, 4, NULL);
#endif
}
