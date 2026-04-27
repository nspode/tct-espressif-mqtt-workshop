
// Programação da função "run", que deve exibir a mensagem "Hello C++" com a variável do contador de quantas vezes o programa foi executado
// Definir nível de log e incluir biblioteca de log
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "hello.h"

namespace HELLO
{
    // Programação da função "run"
    void HelloCpp::run(int i)
    {
        // Exibir mensagem "Hello C++" com o contador de quantas vezes o programa foi executado
        ESP_LOGI("DEBUG", "Hello C++ %d", i);
    }
}
