// Inclusão das bibliotecas do ESP-IDF e do FreeRTOS. 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/event_groups.h>
#include <string>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h" 

// Inclusão de arquivos de configuração e do componente Hello
#include <settings.h>
#include "hello.h"


// Usar o namespace HELLO definido em hello.h
using namespace HELLO;
static const char *TAG = "app";

// Variáveis para sincronização de eventos de Wi-Fi entre tarefas
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0 // Bit para indicar conexão bem-sucedida
#define WIFI_FAIL_BIT BIT1       // Bit para indicar falha de conexão

// Callback executado quando Wi-Fi se conecta com sucesso
void on_wifi_connected()
{
    ESP_LOGW(TAG, "Wi-Fi conectado! ");
}

// Callback executado quando Wi-Fi se desconecta
void on_wifi_disconnected()
{
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
                 PRE_CONFIGURED_WIFI_SSID, PRE_CONFIGURED_WIFI_PASSWORD);
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
    // Criar instância da classe HelloCpp definida no componente
    HelloCpp app;
    int i = 0;

    // Inicializar Wi-Fi em modo estação
    wifi_init_sta();

    // Loop principal: executar a função run() a cada 1 segundo
    while (true)
    {
        app.run(i);
        i++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
