# Projeto: Comunicação MQTT & PokéAPI com ESP32

Documentação do sistema de consulta à PokéAPI via HTTP e troca de dados via protocolo MQTT utilizando o microcontrolador ESP32.

---

## 📌 Visão Geral

O projeto consiste na implementação de uma estação interativa baseada em ESP32 que realiza a consulta de Pokémons diretamente da **PokéAPI (REST)**, exibe as informações em um **Display LCD 20x4**, e integra a comunicação com outros dispositivos através do **protocolo MQTT**.

### Objetivos Principais

* **Consumo de API REST:** Realizar requisições HTTP GET para buscar dados em formato JSON.
* **Comunicação MQTT (Publish/Subscribe):**
* **Publicação de Telemetria:** Envio periódico do status de conexão e dos dados do Pokémon selecionado.
* **Inscrição e Comando:** Recebimento de payloads JSON para controle em tempo real (ex.: alteração do estado do LED RGB).


* **Interface do Usuário:** Navegação interativa por botões físicos e exibição visual em LCD 20x4 I2C.

---

## 🛠️ Tecnologias e Bibliotecas Utilizadas

* **Linguagem / Plataforma:** C++ / Arduino Framework (ESP32)
* **Conectividade Wi-Fi:** `WiFi.h` & `HTTPClient.h`
* **Protocolo MQTT:** `PubSubClient` (Broker público: `broker.emqx.io`)
* **Manipulação de JSON:** `ArduinoJson` (v6/v7 com filtragem de memória `DeserializationOption::Filter`)
* **Interface Visual:** `LiquidCrystal_I2C`
* **Controle de Periféricos:** Biblioteca customizada `Botao.h`

---

## 🏗️ Arquitetura do Sistema

```
                   +------------------------+
                   |     PokéAPI (REST)     |
                   +-----------+------------+
                               |
                               | HTTP GET (JSON)
                               v
+------------------+   HTTP   +------------------+   MQTT (JSON)   +-------------------+
| Botões / Usuário | -------->|    ESP32 Principal| --------------->|   Broker EMQX     |
+------------------+          +--------+---------+                 +---------+---------+
                                       |                                     |
                                       | I2C                                 | MQTT Sub
                                       v                                     v
                              +------------------+                 +-------------------+
                              |  Display LCD 20x4|                 |  Outro ESP32 /    |
                              +------------------+                 |  Dashboard Client |
                                                                   +-------------------+

```

---

## ⚙️ Funcionamento das Funções Principais

### 1. Conexão e Inicialização (`conexao()`)

Conecta o ESP32 à rede Wi-Fi configurada e se inscreve no tópico MQTT de comando. Durante as etapas de reconexão/espera, o LED RGB onboard pisca indicando o status do processo.

### 2. Requisição à API (`pedirParaApi()`)

Envia uma requisição `HTTP GET` para o endpoint `[https://pokeapi.co/api/v2/pokemon/](https://pokeapi.co/api/v2/pokemon/){ID}`. Retorna a resposta completa da API em formato `String`.

### 3. Filtro e Processamento JSON (`buscarNomePokemon()` e `montarTelalcd()`)

Para evitar o estouro da memória RAM ao processar o payload grande da PokéAPI, utiliza-se a técnica de **JSON Filtering** da biblioteca `ArduinoJson`:

* **Navegação (Preview):** Desserializa **apenas** a chave `"name"` para navegação rápida pelos botões.
* **Confirmação (Detalhes):** Desserializa as chaves `"name"`, `"id"` e o array de tipos (`"types"`).

### 4. Publicação de Dados Pokémon (`mqttPokemon()`)

Após a confirmação da seleção, os dados extraídos do Pokémon são estruturados em um novo documento JSON e publicados no tópico:
`senai134/esp32/eduardo-gustavo/pokemon`

**Exemplo de Payload Enviado:**

```json
{
  "usuario": "GustavoRosa",
  "pokemon": "pikachu",
  "pokedex": 25,
  "type1": "electric",
  "type2": ""
}

```

### 5. Telemetria e Heartbeat (`statusConexao()`)

A cada 5 segundos (sem bloquear a execução do `loop` com `delay`), o ESP32 envia um status de funcionamento e conectividade para o tópico:
`senai134/esp32/eduardo-gustavo/status`

**Exemplo de Payload de Telemetria:**

```json
{
  "conectado": true,
  "usuario": "GustavoRosa",
  "IP": "192.168.1.50",
  "timestamp": 1793796662,
  "mensagemErro": "Sem erro"
}

```

### 6. Recepção de Comandos MQTT (`aoReceber()` e `tratarJson()`)

Escuta as mensagens enviadas para o tópico de comando (`senai134/esp32/eduardo-gustavo/comando`). Quando um payload JSON é recebido, extrai a propriedade `estadoLed` e aciona o LED RGB conectado ao pino 48:

**Payload de Exemplo para Ligar o LED:**

```json
{
  "estadoLed": true
}

```

---

## 🎛️ Mapeamento de Hardware

| Componente | Pino / Endereço ESP32 | Função |
| --- | --- | --- |
| **Display LCD I2C** | Endereço `0x27` (SDA/SCL padrão) | Exibição do menu e dados do Pokémon |
| **Botão Cima** | GPIO 11 | Incrementa o ID da Pokédex |
| **Botão Baixo** | GPIO 12 | Decrementa o ID da Pokédex |
| **Botão Enter** | GPIO 13 | Confirma seleção / Consulta API / Publica via MQTT |
| **LED RGB Onboard** | GPIO 48 | Sinalização visual de status e comandos |

---

## 📡 Tópicos MQTT

| Direção | Tópico | Descrição |
| --- | --- | --- |
| **Publicação** | `senai134/esp32/eduardo-gustavo/status` | Envio periódico do status de conexão e IP |
| **Publicação** | `senai134/esp32/eduardo-gustavo/pokemon` | Envio dos dados do Pokémon selecionado |
| **Subscrição** | `senai134/esp32/eduardo-gustavo/comando` | Recebimento de ordens em JSON (ex: ligar/desligar LED) |
