<div style="display: flex; justify-content: center; width: 100%;">
  <div style="position: relative; width: 100%; max-width: 1200px;">
    <img src="../images/techday.png" alt="Tech Day Background" style="width: 100%; display: block; border-radius: 8px;"/>  
  </div>
</div>

<div align="center">

### O que é MQTT?

**Autor: Nelson Spode**

</div>

---

## Introdução

**MQTT** (Message Queuing Telemetry Transport) é um protocolo de comunicação leve, baseado no modelo **publish/subscribe**, projetado para dispositivos com recursos limitados e redes de baixa largura de banda. É amplamente utilizado em aplicações de **IoT** (Internet das Coisas) por sua eficiência e simplicidade.

Foi criado em 1999 por Andy Stanford-Clark (IBM) e Arlen Nipper para monitoramento de oleodutos via satélite — um cenário onde largura de banda era cara e a conexão era instável. Tornou-se padrão OASIS em 2013 e hoje é o protocolo dominante em IoT.

---

## O modelo Publish/Subscribe

Diferente do modelo cliente-servidor tradicional (como HTTP), o MQTT usa o modelo **publish/subscribe**, onde os participantes não se comunicam diretamente entre si. Um intermediário chamado **broker** gerencia toda a comunicação.

```
┌─────────────┐         publica          ┌─────────────────┐
│  Publisher  │ ───────────────────────► │                 │
│  (ESP32-C6) │      tópico + mensagem   │     Broker      │
└─────────────┘                          │     (EMQX)      │
                                         │                 │
┌─────────────┐         recebe           │                 │
│  Subscriber │ ◄─────────────────────── │                 │
│   (MQTTX)   │      tópico + mensagem   │                 │
└─────────────┘                          └─────────────────┘
```

Os participantes se dividem em dois papéis:

- **Publisher:** publica mensagens em um tópico. Não sabe quem vai receber.
- **Subscriber:** se inscreve em tópicos de interesse. Não sabe quem enviou.
- **Broker:** recebe todas as mensagens e as distribui para os subscribers corretos.

Um mesmo dispositivo pode ser publisher e subscriber ao mesmo tempo — como o ESP32-C6 neste workshop, que publica reports e escuta comandos simultaneamente.

---

## Tópicos

Os tópicos são **strings hierárquicas** que organizam as mensagens, similares a caminhos de arquivo. São definidos livremente pela aplicação.

```
/techday/A1B2C3D4E5F6/reports/
/techday/A1B2C3D4E5F6/commands/
/techday/A1B2C3D4E5F6/status/
```

### Wildcards

O MQTT suporta dois caracteres curinga para inscrições:

| Wildcard | Significado | Exemplo |
|---|---|---|
| `+` | Um nível qualquer | `/techday/+/reports/` — reports de qualquer device |
| `#` | Todos os níveis seguintes | `/techday/#` — tudo abaixo de /techday/ |

---

## QoS — Qualidade de Serviço

O MQTT define três níveis de garantia de entrega de mensagens:

| Nível | Nome | Garantia |
|---|---|---|
| **QoS 0** | At most once | Dispara e esquece. Sem confirmação. Pode perder mensagens. |
| **QoS 1** | At least once | Garante entrega, mas pode duplicar mensagens. |
| **QoS 2** | Exactly once | Garante entrega única. Mais lento — usa 4 trocas de mensagem. |

> O AWS IoT Core suporta no máximo **QoS 1**. Usamos QoS 1 para comandos neste workshop.

---

## LWT — Last Will and Testament

O LWT é uma mensagem configurada no momento da conexão, que o broker publica **automaticamente** caso o cliente se desconecte de forma inesperada — sem enviar o pacote `DISCONNECT` adequado.

```
Dispositivo liga     → broker registra LWT ("offline" no tópico de status)
Dispositivo conecta  → publica "online" manualmente
Dispositivo cai      → broker aguarda keepalive × 1.5
                     → broker publica "offline" automaticamente
```

Neste workshop o LWT está configurado em:
```
<MAC>/status/   →  mensagem: "offline"
```

Experimente desconectar o cabo USB e observar o tópico de status no MQTTX.

---

## Keepalive

O keepalive é um intervalo de tempo (em segundos) que define com que frequência o cliente deve enviar um pacote `PINGREQ` ao broker para indicar que ainda está vivo. Se o broker não receber nada do cliente em `keepalive × 1.5` segundos, considera o cliente desconectado e dispara o LWT.

Neste projeto o keepalive está definido em `settings.h`:

```cpp
#define MQTT_KEEPALIVE_S    60  // 60 segundos
```

---

## Pacotes MQTT

O protocolo é binário e compacto. Os principais tipos de pacote são:

| Pacote | Direção | Função |
|---|---|---|
| `CONNECT` | Cliente → Broker | Solicita conexão, envia credenciais e LWT |
| `CONNACK` | Broker → Cliente | Confirma ou recusa a conexão |
| `PUBLISH` | Qualquer direção | Publica uma mensagem em um tópico |
| `SUBSCRIBE` | Cliente → Broker | Solicita inscrição em tópico(s) |
| `PINGREQ` | Cliente → Broker | Keepalive — "estou vivo" |
| `DISCONNECT` | Cliente → Broker | Encerra a conexão controladamente |

---

## MQTT vs HTTP

| | MQTT | HTTP |
|---|---|---|
| Modelo | Publish/Subscribe | Request/Response |
| Overhead | Mínimo (2 bytes de header) | Alto (headers HTTP) |
| Conexão | Persistente | Geralmente por requisição |
| Direção | Bidirecional | Unidirecional (cliente inicia) |
| Ideal para | IoT, telemetria, tempo real | APIs, web, transferência de arquivos |

O header mínimo do MQTT tem apenas **2 bytes** — comparado aos centenas de bytes de um header HTTP. Isso faz diferença em redes com baixa largura de banda ou dispositivos com memória limitada.

---

## Versões do protocolo

| Versão | Ano | Destaques |
|---|---|---|
| MQTT 3.1 | 2010 | Versão original amplamente adotada |
| MQTT 3.1.1 | 2014 | Padrão OASIS, correções e melhorias |
| **MQTT 5.0** | 2019 | Propriedades de mensagem, reason codes, shared subscriptions |

Neste workshop usamos **MQTT 5.0** — a versão mais recente, com suporte nativo no ESP-IDF e no EMQX.

---

## MQTT e segurança

Por padrão, o MQTT não inclui criptografia. A segurança é adicionada em camadas:

| Camada | Mecanismo | Porta padrão |
|---|---|---|
| Sem segurança | MQTT puro | 1883 |
| Transporte criptografado | MQTT sobre TLS | 8883 |
| Autenticação mútua | MQTT sobre mTLS | 8883 |
| Sobre WebSocket | MQTT sobre WS/WSS | 80 / 443 |

Este workshop percorre exatamente essa evolução — do MQTT puro (step/01) ao TLS (step/02) ao mTLS com AWS IoT Core (step/03).

---

## Referências

- [MQTT.org — Especificação oficial](https://mqtt.org/)
- [MQTT 5.0 — OASIS Standard](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
- [ESP-IDF MQTT Client](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/protocols/mqtt.html)
- [EMQX — O que é MQTT](https://www.emqx.com/en/blog/the-easiest-guide-to-getting-started-with-mqtt)

---

<div align="center">
Desenvolvido para o evento <strong>TCT Brasil</strong> pela <strong>Mezzomo e Spode Design House</strong>
</div>