<div style="display: flex; justify-content: center; width: 100%;">
  <div style="position: relative; width: 100%; max-width: 1200px;">
    <img src="images/techday.png" alt="Tech Day Background" style="width: 100%; display: block; border-radius: 8px;"/>  
  </div>
</div>

<div align="left">

**TCT Brasil 2026 — Espressif Systems**

### Step 03 — AWS IoT Core (bônus)

## O que você vai fazer nesta etapa

Nesta etapa bônus vamos conectar a ESP32-C6 diretamente ao **AWS IoT Core** usando **mTLS** — autenticação mútua onde tanto o dispositivo valida o broker quanto o broker valida o dispositivo. Esta é a forma de conexão exigida pela AWS e amplamente usada em produtos IoT em produção.

> **Observação:** a configuração do AWS IoT Core já foi realizada previamente. Esta etapa é demonstrativa — acompanhe o processo de criação no console da AWS e em seguida grave o firmware já configurado na sua board.

## Para completar esta etapa, siga os passos abaixo:

- Entender o que é mTLS e como difere do TLS unilateral do step/02
- Acompanhar a demonstração de criação da Thing no AWS IoT Core
- Atualizar a URI do broker no `settings.h`
- Compilar e gravar — os três certificados já estão embedados no firmware
- Verificar a conexão no monitor serial e no console da AWS
- Conectar o MQTTX ao AWS IoT Core e enviar comandos para o LED

---

## O que mudou em relação ao Step 02

Esta é a diferença fundamental desta etapa — mTLS exige **três arquivos** em vez de um:

**`main.cpp`** — três certificados embedados em vez de um:
```cpp
// Step 02: apenas o CA do broker
extern const uint8_t selfsigned_techday_rootCA_pem_start[] asm("_binary_selfsigned_techday_rootCA_pem_start");

// Step 03: CA da AWS + certificado do dispositivo + chave privada do dispositivo
extern const uint8_t aws_iot_rootCA_pem_start[]        asm("_binary_aws_iot_rootCA_pem_start");
extern const uint8_t aws_iot_rootCA_pem_end[]          asm("_binary_aws_iot_rootCA_pem_end");
extern const uint8_t device_cert_pem_start[]           asm("_binary_device_cert_pem_start");
extern const uint8_t device_cert_pem_end[]             asm("_binary_device_cert_pem_end");
extern const uint8_t device_private_key_pem_start[]    asm("_binary_device_private_key_pem_start");
extern const uint8_t device_private_key_pem_end[]      asm("_binary_device_private_key_pem_end");
```

**`myMqtt.cpp`** — autenticação mútua com certificado e chave do dispositivo:
```cpp
// Step 02: broker valida o cliente apenas pelo CA
mqtt5_cfg.broker.verification.certificate = (const char *)certificate;

// Step 03: broker valida o cliente E o cliente apresenta seu próprio certificado
mqtt5_cfg.credentials.authentication.certificate     = (const char *)device_cert;
mqtt5_cfg.credentials.authentication.certificate_len = device_cert_len;
mqtt5_cfg.credentials.authentication.key             = (const char *)device_private_key;
mqtt5_cfg.credentials.authentication.key_len         = device_private_key_len;
```

> Use `git diff step/02-mqtt-tls step/03-aws-iot` para ver exatamente o que mTLS acrescenta.

---

## TLS unilateral vs mTLS

```
Step 02 — TLS unilateral:
  ESP32-C6  ──► "Você é quem diz ser?"  ──►  Broker EMQX
               valida broker com ca.pem  ◄──  apresenta techday.pem
               broker NÃO valida cliente

Step 03 — mTLS (AWS IoT Core):
  ESP32-C6  ──► "Você é quem diz ser?"  ──►  AWS IoT Core
               valida broker com aws_iot_rootCA.pem
                                         ◄──  "E você?"
               apresenta device_cert.pem ──►
               broker valida o dispositivo com a política IoT
```

No mTLS, **ambos os lados se autenticam**. O AWS IoT Core só aceita conexões de dispositivos que apresentem um certificado válido emitido pela própria AWS e associado a uma política de acesso.

---

## Como o AWS IoT Core foi configurado (demonstração)

### 1. Domínio

No console da AWS, acesse **AWS IoT > Connect > Domain configurations**.

O domínio criado para este evento é:

```
d025025322u2ae4fszdgv-ats.iot.us-east-1.amazonaws.com
```

Este é o endereço a ser configurado como `MQTT_BROKER_URI` no `settings.h`:

```cpp
#define MQTT_BROKER_URI    "d025025322u2ae4fszdgv-ats.iot.us-east-1.amazonaws.com"
```

> Note que para AWS IoT Core o campo usado é `broker.address.hostname` (sem prefixo `mqtts://`), com `transport = MQTT_TRANSPORT_OVER_SSL` e `port = 8883` configurados diretamente no código.

### 2. Criar a Thing

Acesse **AWS IoT > Manage > Things** e clique em **Create things**.

- Selecione **Create single thing** → **Next**
- Nome da thing: `TechDayDevice` → **Next**
- Selecione **Auto-generate a new certificate** → **Next**

### 3. Criar e associar uma política

Antes de finalizar, é necessário associar uma política de acesso ao certificado. Para um dispositivo de testes, uma política permissiva:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:*",
      "Resource": "*"
    }
  ]
}
```

> Em produção, a política deve ser restrita aos tópicos e ações específicas do dispositivo.

### 4. Baixar os certificados

Ao finalizar a criação da thing, a AWS disponibiliza para download:

| Arquivo | Descrição | Renomeado para |
|---|---|---|
| `AmazonRootCA1.pem` | Certificado CA raiz da AWS | `aws_iot_rootCA.pem` |
| `xxxxx-certificate.pem.crt` | Certificado do dispositivo | `device_cert.pem` |
| `xxxxx-private.pem.key` | Chave privada do dispositivo | `device_private_key.pem` |

> **Importante:** a chave privada só pode ser baixada no momento da criação. Se perdida, um novo certificado deve ser gerado.

Estes três arquivos foram incluídos no diretório `main/` do projeto e são embedados automaticamente no firmware durante a compilação via `CMakeLists.txt`.

---

## Infraestrutura do lab

```
┌─────────────────┐       mTLS (8883)         ┌──────────────────────┐
│   ESP32-C6      │ ────────────────────────► │   AWS IoT Core       │
│  (seu device)   │  autenticação mútua       │  us-east-1           │
└─────────────────┘                           └──────────┬───────────┘
         │                                               │
         │  valida AWS com aws_iot_rootCA.pem            ▼
         │  apresenta device_cert.pem        ┌──────────────────────┐
         └──────────────────────────────────►│  Console AWS / MQTTX │
                                             │   (monitoramento)    │
                                             └──────────────────────┘
```

**Broker:** AWS IoT Core  
**Endpoint:** `d025025322u2ae4fszdgv-ats.iot.us-east-1.amazonaws.com`  
**Porta:** `8883` (mTLS)  
**Certificados:** embedados no firmware (`main/`)

---

## Passo 1 — Fazer checkout do branch 

```bash
git checkout step/03-aws-iot
```

---

## Passo 2 — Atualizar a URI do broker

Abra o arquivo `main/settings.h` e atualize a URI:

```cpp
#define MQTT_BROKER_URI    "d025025322u2ae4fszdgv-ats.iot.us-east-1.amazonaws.com"
```

As credenciais Wi-Fi já estão preenchidas dos steps anteriores — nenhuma outra alteração é necessária, visto que os certificados já estão na pasta `main/` .

Dê uma olhada nos arquivos:
- `main/aws_iot_rootCA.pem`
- `main/device_cert.pem`
- `main/device_private_key.pem`

---

## Passo 3 — Compilar e gravar

Selecione a porta serial correta na barra inferior do VSCode (ícone de tomada), depois clique no icone em forma de "fogo". Ao clicar neste ícone, o VSCode irá compilar o projeto, gravar no dispositivo e abrir o monitor serial automaticamente. 

<img src="images/esp-idf-footer.png" alt="VSCode Flash" style="width: 100%; display: block; margin: 20px auto; border-radius: 8px;"/>

Você deve ver no terminal uma sequência semelhante a:

```
I (xxxx) MY_MQTT: Inicializando MQTT para AWS IoT Core...d025025322u2ae4fszdgv-ats.iot.us-east-1.amazonaws.com
I (xxxx) esp-tls: handshake successful
I (xxxx) MY_MQTT: Conectado ao broker MQTT.
I (xxxx) MAIN: Inscrito no tópico: /techday/A1B2C3D4E5F6/commands/
```

> O `vTaskDelay(5000)` no início do `init_mqtt_with_aws_iot_certs` é intencional — aguarda a estabilização do CA store global antes de iniciar a conexão TLS com a AWS.

Para encerrar o monitor: `Ctrl + ]`

---

## Passo 4 — Verificar no console da AWS

Acesse **AWS IoT > Test > MQTT test client** e assine o tópico:

```
/techday/#
```

Você verá os reports publicados pela sua board chegando diretamente no console da AWS.

---

## Passo 5 — Monitorar e enviar comandos pelo MQTTX

O MQTTX pode ser configurado para conectar ao AWS IoT Core usando os mesmos certificados do projeto.

Configure uma nova conexão conforme abaixo:

<div align="center">
  <img src="images/mqttx_setup.png" alt="Configuração MQTTX AWS IoT Core" style="max-width: 900px; border-radius: 8px;"/>
</div>

| Campo | Valor |
|---|---|
| Host | `mqtts://d025025322u2ae4fszdgv-ats.iot.us-east-1.amazonaws.com` |
| Porta | `8883` |
| SSL/TLS | habilitado |
| Certificate | CA or Self signed certificates |
| CA File | `main/aws_iot_rootCA.pem` |
| Client Certificate File | `main/device_cert.pem` |
| Client Key File | `main/device_private_key.pem` |
| MQTT Version | 5.0 |

> ⚠️ **Observação:** estamos usando os mesmos certificados do dispositivo no MQTTX apenas para fins de demonstração. Em um sistema real, cada cliente deve ter seu próprio certificado emitido pela AWS — compartilhar certificados entre dispositivos e clientes de monitoramento não é uma prática recomendada.

### Enviar comandos para o LED

Publique no tópico `/techday/<MAC>/commands/`:

```json
{"command": "set_led", "value": 2}
```

| `value` | Cor do LED |
|---|---|
| `0` | Laranja |
| `1` | Vermelho |
| `2` | Verde |
| `3` | Azul |

---

## Troubleshooting

| Problema | Possível causa | Solução |
|---|---|---|
| `handshake failed` | Certificado expirado ou incorreto | Confirme os arquivos `.pem` na pasta `main/` |
| `MQTT_EVENT_DISCONNECTED` em loop | Política IoT restritiva | Verifique a política associada ao certificado no console AWS |
| `esp_tls_set_global_ca_store` erro | CA store já inicializado | Chame `esp_tls_free_global_ca_store()` antes de reinicializar |
| MQTTX não conecta | Certificados incorretos | Confirme que CA, cert e key correspondem à mesma thing |
| Sem mensagens no console AWS | Tópico incorreto | Confirme o MAC no monitor serial e use `/techday/#` |

---

<div align="center">
Desenvolvido para o evento <strong>TCT Brasil</strong> pela <strong>Mezzomo e Spode Design House</strong>.
</div>