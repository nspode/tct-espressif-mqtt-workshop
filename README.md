<div style="display: flex; justify-content: center; width: 100%;">
  <div style="position: relative; width: 100%; max-width: 1200px;">
    <img src="images/techday.png" alt="Tech Day Background" style="width: 100%; display: block; border-radius: 8px;"/>  
  </div>
</div>

<div align="center">

### From plain MQTT to TLS and AWS IoT Core

**TCT Brasil 2026 — Espressif Systems**

</div>

---

<div align="center">

<img src="images/logo-black.svg" alt="Espressif Systems" style="max-height: 80px; max-width: 600px;"/>

</div>

---

## Sobre este workshop

Hands-on técnico apresentado no evento **TCT Brasil**, com foco em conectividade MQTT utilizando o **ESP32-C6** como kit de desenvolvimento. A sessão percorre três etapas progressivas: conexão sem segurança, conexão com TLS usando certificado autoassinado, e integração com o **AWS IoT Core**.

**Duração:** 70 minutos  
**Nível:** Intermediário  
**Ferramentas:** ESP32-C6 + ESP-IDF + VSCode

---

## Apresentador

**Eng. Nelson Spode**  
_Engenheiro Eletricista, Mestre e Doutor pela UFSM — Universidade Federal de Santa Maria_  
Sócio-gestor — [Mezzomo e Spode](https://www.linkedin.com/in/nelsonspode/) | Design House em Hardware e Software com foco em eletrônica Industrial, IoT e Eletrônica de Potência.

[![LinkedIn](https://img.shields.io/badge/LinkedIn-nelsonspode-0077B5?style=flat&logo=linkedin)](https://www.linkedin.com/in/nelsonspode/)

---

## Pré-requisitos

Antes do evento, instale e configure os itens abaixo. **A configuração do ambiente não será realizada durante o hands-on.**

### Hardware
- Development board **ESP32-C6** (fornecida no evento)
- Cabo USB-C

### Software

| Item | Versão recomendada | Link |
|---|---|---|
| VSCode | ≥ 1.89 | [code.visualstudio.com](https://code.visualstudio.com/) |
| Extensão ESP-IDF (VSCode) | ≥ 1.9 | Marketplace VSCode |
| ESP-IDF | v5.3.1 | Instalado via extensão |
| MQTTX Desktop | Latest | [mqttx.app](https://mqttx.app/) |
| Driver USB | — | CP210x ou CH343 (veja abaixo) |

### Driver USB
- **Windows/Mac:** instale o driver [CP210x](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) ou [CH343](https://github.com/WCHSoftGroup/ch343ser_linux) conforme o chip da sua board
- **Linux:** geralmente já incluído no kernel

### OpenSSL
Necessário para geração do certificado autoassinado na etapa 2.
- **Linux/Mac:** já disponível no terminal
- **Windows:** instale o [Git for Windows](https://gitforwindows.org/) — o Git Bash inclui o `openssl`

---

## Estrutura do repositório

```
tct-espressif-mqtt-workshop/
├── images/
│   ├── logo-tct.png
│   └── logo-espressif.png
├── main/
│   ├── main.c
│   ├── mqtt_handler.c
│   ├── mqtt_handler.h
│   └── CMakeLists.txt
├── certs/                  # Certificados gerados na etapa 2
│   └── .gitkeep
├── CMakeLists.txt
├── sdkconfig.defaults
└── README.md
```

---

## Branches — etapas do hands-on

Cada etapa está em um branch dedicado. Acompanhe o diff entre branches para entender exatamente o que muda a cada evolução.

| Branch | Etapa | Descrição |
|---|---|---|
| `main` | — | Este README e estrutura base do projeto |
| `step/01-mqtt-plain` | Etapa 1 | Conexão MQTT sem TLS — porta 1883 |
| `step/02-mqtt-tls` | Etapa 2 | Conexão MQTT com TLS — porta 8883, certificado autoassinado |
| `step/03-aws-iot` | Etapa 3 (bônus) | Integração com AWS IoT Core — mTLS |

> **Dica:** use `git diff step/01-mqtt-plain step/02-mqtt-tls` para visualizar exatamente o que TLS exige a mais no cliente.

---

## Infraestrutura do lab

```
┌─────────────────┐        MQTT         ┌──────────────────┐
│   ESP32-C6      │ ──────────────────► │   Broker EMQX    │
│  (seu device)   │   porta 1883/8883   │  (VM local / VPS)│
└─────────────────┘                     └────────┬─────────┘
                                                  │
                                                  ▼
                                        ┌──────────────────┐
                                        │      MQTTX       │
                                        │  (monitoramento) │
                                        └──────────────────┘
```

**Broker principal:** EMQX rodando em VM local (apresentador)  
**Broker backup:** instância em VPS (disponibilizado durante o evento)  
**Cliente de monitoramento:** MQTTX Desktop

---

## Etapa 1 — MQTT sem TLS (porta 1883)

> Branch: `step/01-mqtt-plain`

### O que você vai fazer
- Clonar o branch e abrir no VSCode
- Configurar SSID/senha Wi-Fi e URI do broker via `menuconfig`
- Compilar, fazer flash e monitorar via `idf.py flash monitor`
- Verificar a conexão e as mensagens no MQTTX

### Configuração (`idf.py menuconfig`)
```
Example Configuration
  ├── WiFi SSID
  ├── WiFi Password
  └── MQTT Broker URI    →  mqtt://<IP_DO_BROKER>:1883
```

---

## Etapa 2 — MQTT com TLS (porta 8883)

> Branch: `step/02-mqtt-tls`

### O que você vai fazer
- Gerar um certificado autoassinado com `openssl`
- Embedar o certificado CA no firmware via `CMakeLists.txt`
- Configurar o cliente MQTT para usar TLS
- Verificar a conexão segura no MQTTX

### Geração do certificado
```bash
# Gerar chave privada e certificado CA autoassinado
openssl req -new -x509 -days 365 -extensions v3_ca \
  -keyout certs/ca.key -out certs/ca.crt
```

### O que muda no código em relação à etapa 1
- URI: `mqtt://` → `mqtts://`
- Porta: `1883` → `8883`
- Campo adicionado em `esp_mqtt_client_config_t`:
```c
.broker.verification.certificate = (const char *)ca_crt_start,
```

---

## Etapa 3 — AWS IoT Core (bônus)

> Branch: `step/03-aws-iot`

### O que você vai fazer
- Criar um *thing* no AWS IoT Core
- Baixar os certificados gerados pela AWS (CA, client cert, client key)
- Configurar o endpoint e os três certificados no firmware
- Verificar a conexão no MQTTX e no console da AWS

### O que muda em relação à etapa 2
- Broker: EMQX → endpoint AWS (`xxxxxxxx.iot.<region>.amazonaws.com`)
- Autenticação: TLS unilateral → **mTLS** (o broker também valida o cliente)
- Campos adicionados:
```c
.client_cert_pem = (const char *)client_crt_start,
.client_key_pem  = (const char *)client_key_start,
```

> **Nota:** TLS unilateral (etapa 2) = só o cliente valida o broker.  
> mTLS (etapa 3) = validação mútua — broker e cliente se autenticam.

---

## Troubleshooting

| Problema | Possível causa | Solução |
|---|---|---|
| Porta COM não aparece | Driver USB não instalado | Instale CP210x ou CH343 |
| `idf.py flash` falha | Porta ocupada ou permissão | Feche o monitor; no Linux: `sudo usermod -aG dialout $USER` |
| Wi-Fi não conecta | SSID/senha errados | Revise via `menuconfig` |
| `mbedtls` erro de certificado | Certificado expirado ou CN errado | Regere o certificado com o IP/hostname correto no CN |
| MQTTX não recebe mensagens | Topic incorreto | Confirme o topic no código e no MQTTX (case-sensitive) |

---

## Referências

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/)
- [ESP-IDF MQTT Client](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/protocols/mqtt.html)
- [EMQX Documentation](https://docs.emqx.com/)
- [AWS IoT Core Developer Guide](https://docs.aws.amazon.com/iot/latest/developerguide/)
- [MQTTX](https://mqttx.app/)

---

<div align="center">

Desenvolvido para o evento **TCT Brasil** em parceria com a **Espressif Systems**

</div>