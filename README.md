<div style="display: flex; justify-content: center; width: 100%;">
  <div style="position: relative; width: 100%; max-width: 1200px;">
    <img src="images/techday.png" alt="Tech Day Background" style="width: 100%; display: block; border-radius: 8px;"/>  
  </div>
</div>

<div align="left">

**TCT Brasil 2026 — Espressif Systems**

### Step 02 — MQTT com TLS (porta 8883)

## O que você vai fazer nesta etapa

Nesta etapa vamos adicionar segurança à conexão MQTT usando TLS com um certificado autoassinado. O certificado já está incluído no repositório e será embarcado diretamente no firmware.

> **Durante a apresentação:** O certificado pode será gerado ao vivo como demonstração. Caso o tempo esteja curto, o certificado já configurado no repositório será utilizado diretamente.

## Para completar esta etapa, siga os passos abaixo:

- Fazer checkout deste branch
- Atualizar a URI do broker no `settings.h` para usar `mqtts://` na porta `8883`
- Compilar e gravar — o certificado já está embedado no firmware
- Verificar no monitor serial que a conexão TLS foi estabelecida
- Verificar no MQTTX que as mensagens continuam chegando, agora de forma segura
- Explorar o diff entre este branch e o anterior

---

## O que mudou em relação ao Step 01

Esta é a essência desta etapa — três mudanças no código que habilitam TLS:

**`settings.h`** — URI atualizada:
```cpp
// Antes (step/01):
#define MQTT_BROKER_URI    "mqtt://ec2-3-80-250-87.compute-1.amazonaws.com:1883"

// Agora (step/02):
#define MQTT_BROKER_URI    "mqtts://ec2-3-80-250-87.compute-1.amazonaws.com:8883"
```

**`main.cpp`** — certificado embedado no firmware:
```cpp
// Referências ao certificado CA embedado via CMakeLists.txt
extern const uint8_t selfsigned_techday_rootCA_pem_start[] asm("_binary_selfsigned_techday_rootCA_pem_start");
extern const uint8_t selfsigned_techday_rootCA_pem_end[]   asm("_binary_selfsigned_techday_rootCA_pem_end");
```

**`myMqtt.cpp`** — campo adicionado na configuração do cliente:
```cpp
// O cliente agora valida a identidade do broker usando o certificado CA
mqtt5_cfg.broker.verification.certificate = (const char *)certificate;
```

> Use `git diff step/01-mqtt-plain step/02-mqtt-tls` para visualizar exatamente o que mudou.
> Ou utiilize a extensão **Git Graph** do VSCode para uma comparação visual.

---

## Infraestrutura do lab

```
┌─────────────────┐       MQTTS (8883)        ┌──────────────────────┐
│   ESP32-C6      │ ────────────────────────► │   Broker EMQX        │
│  (seu device)   │     com TLS/SSL           │  AWS EC2             │
└─────────────────┘                           └──────────┬───────────┘
         │                                               │
         │  valida identidade                            ▼
         │  do broker usando                  ┌──────────────────────┐
         └─── ca.pem embedado ───────────────►│        MQTTX         │
                                              │   (monitoramento)    │
                                              └──────────────────────┘
```

**Broker:** EMQX rodando em instância AWS EC2  
**Endereço:** `ec2-3-80-250-87.compute-1.amazonaws.com`  
**Porta:** `8883` (com TLS)  
**Certificado:** `selfsigned_techday_rootCA.pem` — embedado no firmware  (pode ser encontrado na pasta `main/`)
**Cliente de monitoramento:** MQTTX Desktop

### Dashboard EMQX

**URL:** [http://ec2-3-80-250-87.compute-1.amazonaws.com:18083/#/dashboard/overview](http://ec2-3-80-250-87.compute-1.amazonaws.com:18083/#/dashboard/overview)

| Campo | Valor |
|---|---|
| Usuário | `TCT` |
| Senha | `Techday_2026` |

> Verifique as conexões ativas na aba `Listners` no Dashboard. Verá a conexão do tipo **SSL** na porta `8883`.
---

## Como o certificado funciona neste projeto

O certificado CA (`selfsigned_techday_rootCA.pem`) é embedado diretamente no firmware durante a compilação via `CMakeLists.txt`:

```cmake
target_add_binary_data(__idf_main selfsigned_techday_rootCA.pem BINARY)
```

O ESP-IDF gera automaticamente dois símbolos que referenciam o início e o fim do certificado na memória flash — sem necessidade de sistema de arquivos ou cartão SD.

Quando o cliente MQTT se conecta ao broker, o ESP32-C6 usa este certificado para **validar a identidade do broker** antes de estabelecer a conexão. Se o certificado do broker não for assinado pela mesma CA, a conexão é recusada.

```
ESP32-C6 ──► "Você é quem diz ser?" ──► Broker EMQX
             usa ca.pem para validar ◄── apresenta techday.pem
```

---

## Como o certificado foi gerado (demonstração)

> Esta seção descreve o processo executado ao vivo durante a apresentação. Para referência detalhada, consulte [docs/ssl_certificate.md](docs/ssl_certificate.md).

### Passo 1 — Gerar a chave privada e o certificado CA

```bash
# Gerar a chave privada da CA
openssl genrsa -out ca.key 2048

# Gerar o certificado raiz autoassinado (válido por 10 anos)
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -out ca.pem
```

### Passo 2 — Gerar a chave e o certificado para o servidor

Criar o arquivo `openssl.cnf`:

```ini
[req]
default_bits  = 2048
distinguished_name = req_techday
req_extensions = req_ext
x509_extensions = v3_req
prompt = no
[req_techday]
countryName = BR
stateOrProvinceName = São Paulo
localityName = São Paulo
organizationName = TCT
commonName = ec2-3-80-250-87.compute-1.amazonaws.com
[req_ext]
subjectAltName = @alt_names
[v3_req]
subjectAltName = @alt_names
[alt_names]
IP.1 = 3.80.250.87
DNS.1 = ec2-3-80-250-87.compute-1.amazonaws.com
```

```bash
# Gerar a chave privada do servidor
openssl genrsa -out techday.key 2048

# Gerar a solicitação de certificado (CSR)
openssl req -new -key ./techday.key -config openssl.cnf -out techday.csr

# Assinar o CSR com o certificado CA (válido por 10 anos)
openssl x509 -req -in ./techday.csr -CA ca.pem -CAkey ca.key \
  -CAcreateserial -out techday.pem -days 3650 -sha256
```

### Passo 3 — Configurar o broker EMQX

O `techday.pem` e o `techday.key` são configurados no broker EMQX para habilitar TLS na porta `8883`. O `ca.pem` é renomeado para `selfsigned_techday_rootCA.pem` e incluído no repositório para ser embedado no firmware.



---

## Passo 1  — Fazer checkout do branch (se já não o fez)

```bash
git checkout step/02-mqtt-tls
```

## Passo 2 — Atualizar a URI do broker

Abra o arquivo `main/settings.h` e atualize a URI:

```cpp
#define MQTT_BROKER_URI    "mqtts://ec2-3-80-250-87.compute-1.amazonaws.com:8883"
```

As credenciais Wi-Fi já estão preenchidas do step anterior — nenhuma outra alteração é necessária.

---

## Passo 3 — Compilar e gravar

> **Caso queira compilar com a versão 6.0.1**: No ESP-IDF 6.0.x, o ESP-MQTT foi retirado do repositório do IDF e passou a ser um componente gerido pelo **IDF Component Manager**, com o identificador espressif/mqtt.  
>
> Então, para usar a versão 6.0.1 do ESP-IDF (**somente**), é necessário adicionar a dependência do componente MQTT (e também do cjson) no arquivo `idf_component.yml`. o arquivo do projeto deverá ficar assim:
>
>```yaml
>dependencies:
>  espressif/led_strip: ">=2.5.0"
>  espressif/mqtt: "^1.0.0"
>  espressif/cjson: "^1.7.19"
>  idf:
>    version: ">=5.0"
>```
>
> O arquivo CMakeLists.txt da pasta `main` também precisa ser atualizado para substituir o  componente json pelo cjson:
>
> ```cmake
> idf_component_register(SRCS "main.cpp"
>                       INCLUDE_DIRS "."
>                       PRIV_REQUIRES  hello esp_wifi esp_driver_rmt esp_netif esp_event >myMqtt cjson led_strip )
> ```

Selecione a porta serial correta na barra inferior do VSCode (ícone de tomada), depois clique no icone em forma de "fogo". Ao clicar neste ícone, o VSCode irá compilar o projeto, gravar no dispositivo e abrir o monitor serial automaticamente. 

<img src="images/esp-idf-footer.png" alt="VSCode Flash" style="width: 100%; display: block; margin: 20px auto; border-radius: 8px;"/>

Você deve ver no terminal uma sequência semelhante a:

```
I (xxxx) MY_MQTT: Inicializando MQTT...mqtts://ec2-3-80-250-87.compute-1.amazonaws.com:8883
I (xxxx) esp-tls: handshake successful
I (xxxx) MY_MQTT: Conectado ao broker MQTT.
I (xxxx) MAIN: Inscrito no tópico: /techday/A1B2C3D4E5F6/commands/
```

> A linha `handshake successful` confirma que o TLS foi estabelecido com sucesso.

---

## Passo 3 — Verificar no MQTTX

Crie uma nova conexão no MQTTX apontando para a porta `8883`:

| Campo | Valor |
|---|---|
| Host | `mqtts://ec2-3-80-250-87.compute-1.amazonaws.com` |
| Porta | `8883` |
| SSL/TLS | habilitado |
| CA File | disponivel na pasta docs/ |

O arquivo `ca.pem` esta na disponivel na pasta docs/

A configuraçào final deverá ficar como na figura abaixo:
<img src="images/mqttx_setup.png" alt="MQTX Connection" style="width: 100%; display: block; margin: 20px auto; border-radius: 8px;"/>

Os tópicos e comandos são os mesmos do step anterior:

| Tópico | O que você vai ver |
|---|---|
| `/techday/<MAC>/reports/` | Reports a cada 5 segundos |
| `<MAC>/status/` | Status online/offline |
| `#` | Todos os tópicos do lab |

O comportamento da aplicação é idêntico ao step/01 — a diferença está no canal de comunicação, agora criptografado.

---

## O que está acontecendo agora

```
Step 01:  ESP32-C6 ──── texto puro ────────────────► Broker
Step 02:  ESP32-C6 ──── dados criptografados ──────► Broker
                    TLS valida identidade do broker
```

> **IMPORTANTE:**  **TLS unilateral** — Neste exemplo, apenas o cliente valida o broker. O broker não valida o cliente. Na etapa 3 (bônus), utlizaremos um certificado para o cliente ESP32-C6 utilizado o AWS IoT Core.

---

## Troubleshooting

| Problema | Possível causa | Solução |
|---|---|---|
| `mbedtls` erro no handshake | URI ainda com `mqtt://` | Troque para `mqtts://` no `settings.h` |
| `certificate verify failed` | Certificado do broker diferente do embedado | Confirme que está usando o `ca.pem` correto |
| `handshake timeout` | Porta 8883 bloqueada na rede | Verifique firewall ou tente a rede do celular |
| Conexão cai em loop | `commonName` do certificado não bate com o host | Verifique o CN do certificado gerado |
| MQTTX não conecta na 8883 | SSL não habilitado no MQTTX | Habilite SSL/TLS nas configurações da conexão |

---

## Próximo passo

Bônus: se houver tempo, avance para a etapa 3 onde vamos conectar ao AWS IoT Core usando mTLS — autenticação mútua entre dispositivo e broker:

```bash
git checkout step/03-aws-iot
```

---

<div align="center">
Desenvolvido para o evento <strong>TCT Brasil</strong> pela <strong>Mezzomo e Spode Design House</strong>
</div>