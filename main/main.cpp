// Inclusão das bibliotecas do ESP-IDF e do FreeRTOS. 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/event_groups.h>
#include "esp_log.h"
#include "esp_wifi.h" 
#include "esp_mac.h" // Include for esp_read_mac
#include "led_strip.h"

// Bibliotecas C++ 
#include <string>
#include <string.h>
#include "cJSON.h"

// Arquivos de configuração e de componentes customizados.
#include <settings.h> // Configurações deste projeto
#include "myMqtt.h" // Componente MQTT Customizado 
#include "main.h" // Declarações de funções e variáveis globais para main.cpp


static const char *TAG = "app"; // Tag para logs

// Variáveis globais para MQTT e LED
std::string report_topic = "";  // Topico de exemplo para enviar reports atraves do MQTT
std::string command_topic = ""; // Topico de exemplo para receber comandos atraves do MQTT
static bool g_mqtt_is_initialized = false; // Flag para indicar se MQTT foi inicializado
uint8_t ledStatus = 0; // Variável para armazenar o status atual do LED (0, 1, 2, 3)
static led_strip_handle_t led_strip = nullptr; // Handle do LED — variável global

extern const uint8_t selfsigned_techday_rootCA_pem_start[] asm("_binary_selfsigned_techday_rootCA_pem_start");
extern const uint8_t selfsigned_techday_rootCA_pem_end[] asm("_binary_selfsigned_techday_rootCA_pem_end");

// Event group para gerenciar o estado da conexão Wi-Fi
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0 // Bit para indicar conexão bem-sucedida
#define WIFI_FAIL_BIT BIT1       // Bit para indicar falha de conexão


// ─── LED ────────────────────────────────────────────────────────────────────
// Implementação das funções para controle do LED usando a biblioteca led_strip do ESP-IDF.
void led_init()
{
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = 8;
    strip_config.max_leds = 1;
    strip_config.led_model = LED_MODEL_WS2812;    

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10 MHz

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}

void led_set(uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
}

// ─── MQTT ───────────────────────────────────────────────────────────────────
// Callback executado quando a conexão MQTT é estabelecida com sucesso. 
//Aqui podemos realizar ações como se inscrever em tópicos e publicar status online do dispositivo.
void mqtt_connected_handler(const char *broker_uri)
{
    ESP_LOGI("MAIN", "Conectado ao broker: %s", broker_uri);

    // Inscrever-se no tópico de comando para receber comandos do AWS IoT Core
    MYMQTT::mqtt_subscribe_topic(command_topic.c_str(), 1); // max qos for AWS Core IoT is 1
    
    ESP_LOGI("MAIN", "Inscrito no tópico: %s", command_topic.c_str());
    
    // Publicar status online do dispositivo usando o device ID como parte do tópico
    std::string device_id = get_device_id();
    if (!device_id.empty())
    {
        MYMQTT::publish_status(device_id.c_str(), "online");
    }
}

// Callback para lidar com mensagens recebidas via MQTT
// Estamos usando cJSON para parsear mensagens JSON recebidas e extrair comandos e valores.

void mqtt_message_handler(const char *topic, const char *message)
{
    ESP_LOGI("MAIN", "Mensagem recebida - Tópico: %s, Mensagem: %s", topic, message);

    // Verificar se a mensagem é do tópico de comando
    if (std::string(topic) == command_topic)
    {
        // Parse the JSON message
        cJSON *root = cJSON_Parse(message);
        if (root == NULL)
        {
            ESP_LOGE("MAIN", "Erro ao fazer parsing da mensagem JSON");
            return;
        }

        // Extrai os campos "command" e "value" do JSON
        cJSON *command = cJSON_GetObjectItem(root, "command");
        cJSON *value = cJSON_GetObjectItem(root, "value");
        
        // Verifica se o campo "command" é válido
        if (command == NULL || !cJSON_IsString(command))
        {
            ESP_LOGE("MAIN", "Campo 'command' não encontrado ou inválido");
            cJSON_Delete(root); // Liberar memória antes de retornar
            return;
        }

        ESP_LOGI("MAIN", "Comando recebido: %s", command->valuestring);

        // Processa o comando recebido
        if (std::string(command->valuestring) == "set_led")
        {
            if (value && cJSON_IsNumber(value)) // Check if "value" is a valid number
            {
                ESP_LOGE("MAIN", "TYPECAST");
                ledStatus = static_cast<uint8_t>(value->valueint);

                // Atualiza o LED com base no valor recebido
                if(ledStatus == 0){
                    led_set(255, 50, 0); // Laranja
                } else if (ledStatus == 1){
                    led_set(255, 0, 0); // Laranja 
                } else if (ledStatus == 2){
                    led_set(0, 255, 0); // Verde 
                } else if (ledStatus == 3){
                    led_set(0, 0, 255); // Azul
                } else {
                    ESP_LOGE("MAIN", "Valor inválido para 'set_led': %d", value->valueint);
                }
                
            }
            else
            {
                ESP_LOGE("MAIN", "Valor inválido para 'set_mode'");
            }       
        
        }
        else
        {
            ESP_LOGE("MAIN", "Comando desconhecido: %s", command->valuestring);
        }

        // Liberar memória alocada para o JSON
        cJSON_Delete(root);
        // Publicar o estado atual após processar o comando
        publishCurrentState();
    }
}

// Função para publicar alguma coisa...
// Neste exemplo, estamos publicando o status atual do LED e uma mensagem de texto usando o tópico de relatório configurado.
void publishCurrentState()
{
    // Send report via MQTT
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ledStatus", ledStatus);
    cJSON_AddStringToObject(root, "message", "Tech Day Road Show 2026 - ESPRESSIF WORKSHOP");
    

    char *json_string = cJSON_PrintUnformatted(root);
    MYMQTT::publish_message(report_topic.c_str(), json_string);
    cJSON_Delete(root);
    free(json_string);
}

// ─── Wi-Fi ─────────────────────────────────────────────────────────────────
// Função para obter o device ID do dispositivo.
// Estamos usando o endereço MAC do dispositivo para construir a estrurura de tópicos MQTT, 
// garantindo que cada dispositivo tenha um tópico único. 
std::string get_device_id()
{
    // Ler o endereço MAC do dispositivo usando a função esp_read_mac
    uint8_t mac[6];
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao ler MAC address: %s", esp_err_to_name(err));
        return "";
    }

    // Converter o endereço MAC para uma string legível (ex: "A1B2C3D4E5F6")
    char mac_str[18];    
    // Formata o endereço MAC como uma string hexadecimal sem separadores (ex: "A1B2C3D4E5F6")
    snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ESP_LOGI(TAG, "Device ID (MAC): %s", mac_str);
    // Retorna o device ID como uma string
    return std::string(mac_str);
}

// Callback executado quando Wi-Fi se conecta com sucesso
void on_wifi_connected()
{
    ESP_LOGW(TAG, "Wi-Fi conectado! Iniciando MQTT...");

        // Busca o endereço MAC do dispositivo e constrói os tópicos MQTT usando o device ID.
        std::string device_id = get_device_id();
        if (!device_id.empty())
        {
            ESP_LOGI(TAG, "Configurando tópicos MQTT com deviceId: %s", device_id.c_str());
            report_topic = "/techday/" + device_id + "/reports/";            
            command_topic = "/techday/" + device_id + "/commands/";            
        }
        else
        {
            ESP_LOGW(TAG, "Device ID não encontrado na NVS. Usando DUMMY_DEVICE_ID.");
            report_topic = "/techday/" + std::string(DUMMY_DEVICE_ID) + "/reports/";            
            command_topic = "/techday/" + std::string(DUMMY_DEVICE_ID) + "/commands/";
        }

        // Uma vez conectado ao Wi-Fi, inicializamos o MQTT usando a URI do broker e o device ID para autenticação.
        MYMQTT::init_mqtt_with_selfsigned_cert(MQTT_BROKER_URI, selfsigned_techday_rootCA_pem_start, device_id.c_str());

        // Registramos o callback para lidar com eventos de conexão MQTT (ex: quando a conexão é estabelecida com sucesso)
        MYMQTT::register_mqtt_connected_callback(mqtt_connected_handler);
        // Registramos os callbacks para lidar com mensagens recebidas via MQTT (ex: comandos do AWS IoT Core)
        MYMQTT::register_mqtt_callback(mqtt_message_handler);

        // Definimos a flag global para indicar que o MQTT foi inicializado com sucesso, permitindo que outras partes do código saibam que podem usar o MQTT para publicar mensagens.
        g_mqtt_is_initialized = true;
        ESP_LOGI(TAG, "MQTT inicializado com sucesso!");
}

// Callback executado quando Wi-Fi se desconecta
void on_wifi_disconnected()
{
    // Apenas logamos a desconexão. 
    // O sistema continuará tentando reconectar automaticamente via event_handler.
    ESP_LOGW(TAG, "Wi-Fi desconectado!");
}

// Handler de eventos que processa eventos de Wi-Fi e IP
// Chamado automaticamente pelo sistema quando eventos ocorrem
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // Quando a estação Wi-Fi inicia, tenta conectar ao Access Point
        ESP_LOGI(TAG, "Estação Wi-Fi iniciada");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Se desconectar, tenta reconectar
        esp_wifi_connect();
        ESP_LOGI(TAG, "Tentando conectar novamente ao AP");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // Quando recebe um IP, a conexão foi bem-sucedida
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP obtido:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        on_wifi_connected();
    }
}

// Função para inicializar o Wi-Fi em modo estação (STA)
void wifi_init_sta(void)
{
    
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Registrar handlers para eventos de Wi-Fi e IP
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Configurar SSID, senha e autenticação
    // Aqui foi necessário modificar o exemplo para compatibilidade com estilo C++.
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, PRE_CONFIGURED_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, PRE_CONFIGURED_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    // Iniciar Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finalizado.");

    // Aguardar conexão com timeout de 10 segundos
    /* Aguardando até que a conexão seja estabelecida (WIFI_CONNECTED_BIT) ou timeout ocorra.
     * Usando timeout em vez de portMAX_DELAY para evitar bloqueio se WiFi indisponível.
     * WiFi continuará tentando reconectar em background automaticamente via event_handler. */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(WIFI_INIT_TIMEOUT_MS));

    /* xEventGroupWaitBits() retorna os bits antes da chamada retornar, portanto podemos testar qual evento
     * realmente aconteceu. */
    // Verificar resultado da conexão
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Conectado ao AP SSID: %s",
                 PRE_CONFIGURED_WIFI_SSID);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGW(TAG, "Falha ao conectar em SSID:%s, password:%s (tentará novamente em background)",
                 PRE_CONFIGURED_WIFI_SSID, PRE_CONFIGURED_WIFI_PASSWORD);
    }
    else
    {
        // Timeout ocorreu - WiFi não se conectou no período de timeout
        ESP_LOGW(TAG, "Timeout de conexão WiFi após %d ms. SSID:%s. O sistema continuará inicializando. WiFi tentará reconectar em background.",
                 WIFI_INIT_TIMEOUT_MS, PRE_CONFIGURED_WIFI_SSID);
        ESP_LOGI(TAG, "Sistema continuando sem WiFi. Reconexão automática continuará em background.");
    }
}

extern "C" void app_main(void)
{
    
    // Inicializar o LED
    led_init();
    led_set(255, 50, 0); // Laranja — inicializando

    // Inicializar Wi-Fi em modo estação
    wifi_init_sta();

    // Loop principal: publicar o estado atual do dispositivo a cada 5 segundos.
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
        publishCurrentState();
    }
}
