/* 
PROJETO: COMUNICACAO MQTT ENTRE ESP32
DESCRICAO: COMUNICAR COM OUTRO ESP32 PELO MQTT
NOME: EDUARDO DOS SANTOS BABETTO 
DATA: 05/11/2026
VERSAO: 0.1
*/
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Botao.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>

const char* SSID = "SENAI EDUCACIONAL";
const char* SENHA = "info@134";

const char* BROKER = "broker.emqx.io";
const int PORTA = 1883;

const char* TOPICO_SUB = "senai134/esp32/eduardo-gustavo/comando";
const char* TOPICO_PUB = "senai134/esp32/eduardo-gustavo/status";
const char *TOPICO_POKEMON= "senai134/esp32/eduardo-gustavo/pokemon";

const String enderecoAPI = "https://pokeapi.co/api/v2/pokemon/";

LiquidCrystal_I2C lcd(0x27, 20, 4);

Botao btnCima(11);
Botao btnBaixo(12);
Botao btnEnter(13);

String nomePokemon = "";
int pokedex = 1;
String typesPokemon[2] = {"", ""};

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String pedirParaApi(int id);
void buscarNomePokemon(int id);
bool montarTelalcd(int id);
void aoReceber(char* topico, byte* payload, unsigned int tamanho);
void tratarJson(String mensagem);
void telaInicial();
void statusConexao();
void conexao();
void mqttPokemon();

void setup() {
    Serial.begin(115200);

    lcd.init();
    lcd.backlight();

    conexao();

    btnCima.iniciar();
    btnBaixo.iniciar();
    btnEnter.iniciar();

    lcd.clear();
    telaInicial();
}

void loop() {
    mqtt.loop();
    statusConexao();

    btnBaixo.atualizar();
    btnCima.atualizar();
    btnEnter.atualizar();

    // Navegação no Menu (Aumenta ID)
    if (btnCima.pressionou()) {
        if (pokedex < 1025) pokedex++;
        buscarNomePokemon(pokedex);
        telaInicial();
    }

    // Navegação no Menu (Diminui ID)
    if (btnBaixo.pressionou()) {
        if (pokedex > 1) pokedex--;
        buscarNomePokemon(pokedex);
        telaInicial();
    }

    // Confirmação: Mostra a Tela de Detalhes Completa
    if (btnEnter.pressionou()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Carregando...");

        if (montarTelalcd(pokedex)) {
            mqttPokemon(); // Publica os dados completos no broker MQTT
        } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Erro ao carregar!");
            delay(1500);
            telaInicial();
        }
    }
}

void aoReceber(char* topico, byte* payload, unsigned int tamanho) {

    String mensagem;
    mensagem.reserve(tamanho);

    for (unsigned int i = 0; i < tamanho; i++) {
        mensagem += (char)payload[i];
    }

    Serial.printf("Mensagem recebida no tópico: %s\n", topico);
    Serial.println(mensagem);

    rgbLedWrite(48, 0, 0, 0);
    delay(500);

    rgbLedWrite(48, 0, 0, 255);
    delay(200);

    tratarJson(mensagem);
}

void tratarJson(String mensagem) {

    JsonDocument doc;

    DeserializationError codigoErro = deserializeJson(doc, mensagem);

    if (codigoErro) {
        Serial.println("Erro ao interpretar JSON!");
        return;
    }

    bool estadoLed = doc["estadoLed"].as<bool>();

    if (estadoLed) {
        Serial.println("LED LIGADO");
        rgbLedWrite(48, 0, 0, 255);
    }
    else {
        Serial.println("LED DESLIGADO");
        rgbLedWrite(48, 0, 0, 0);
    }
}

void statusConexao() {

    const unsigned long tempoEntreEnvios = 5000;
    static unsigned long ultimoEnvio = 0;

    bool conectado = mqtt.connected();

    const char* usuario = "GustavoRosa";

    IPAddress IP = WiFi.localIP();

    unsigned int timestamp = 1793796662;

    String mensagemErro = "Sem erro";

    if (millis() - ultimoEnvio >= tempoEntreEnvios) {

        ultimoEnvio = millis();

        JsonDocument doc;

        doc["conectado"] = conectado;
        doc["usuario"] = usuario;
        doc["IP"] = IP.toString();
        doc["timestamp"] = timestamp;
        doc["mensagemErro"] = mensagemErro;

        String mensagem;

        serializeJson(doc, mensagem);

        if (mqtt.connected()) {
            mqtt.publish(TOPICO_PUB, mensagem.c_str());
        }

        Serial.printf(
            "Mensagem enviada no tópico %s:\n%s\n",
            TOPICO_PUB,
            mensagem.c_str()
        );
    }
}

//* ===============================DISPLAY===============================
void telaInicial() {
    lcd.setCursor(0, 0);
    lcd.printf("SELECIONE ID: %-4d ", pokedex);
    
    lcd.setCursor(0, 1);
    lcd.printf("NOME: %-14s", nomePokemon.c_str());

    lcd.setCursor(0, 2);
    lcd.print("ENTER = CONFIRMAR   ");
}
//* =====================================================================

//* ===============================DISPLAY===============================
void conexao() {

    String id = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conectando WiFi...");

    //! =================SERIAL==================
    Serial.printf("Conectando na rede %s", SSID);

    WiFi.begin(SSID, SENHA);

    rgbLedWrite(48, 255, 0, 0);

    while (WiFi.status() != WL_CONNECTED) {

      Serial.print(".");
    //! ==========================================

      delay(500);

      rgbLedWrite(48, 0, 0, 0);
      delay(500);

      rgbLedWrite(48, 255, 255, 0);
      delay(500);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi conectado!");

    //! ===================SERIAL===================
    Serial.printf("\nWiFi conectado com sucesso!\n");
    //! ============================================

    rgbLedWrite(48, 255, 255, 0);

    delay(1500);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conectando Broker:");
    lcd.setCursor(0, 1);
    lcd.print(BROKER);

    //! ===================SERIAL===================
    Serial.printf("Conectando ao Broker %s\n", BROKER);
    //! ============================================

    mqtt.setServer(BROKER, PORTA);
    mqtt.setCallback(aoReceber);

    bool conectadoAoBroker = mqtt.connect(id.c_str());

    if (conectadoAoBroker) {

        mqtt.subscribe(TOPICO_SUB);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Broker conectado!");

        lcd.setCursor(0, 1);
        lcd.print("MQTT conectado!");

        //! =========================SERIAL=========================
        Serial.printf("Conectado ao broker %s com sucesso!\n", BROKER);
        Serial.printf("Inscrito no tópico: %s\n", TOPICO_SUB);
        //! ========================================================

        rgbLedWrite(48, 0, 255, 0);

    }
    else {

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Falha no Broker!");

        //! =========================SERIAL=========================
        Serial.printf("Falha ao conectar ao broker %s\n", BROKER);
        Serial.print("Falha: ");
        Serial.println(mqtt.state());
        //! ========================================================

        rgbLedWrite(48, 255, 0, 0);

        delay(2000);
    }

    delay(1500);
}
//* ===============================DISPLAY===============================


String pedirParaApi(int codigo) {
  HTTPClient http;
  http.begin(enderecoAPI + codigo);

  int codigoHTTP = http.GET(); //* Retorna o codigo HTTP ex.:200 -> OK ou 404 -> Not found

  String resposta;

  if (codigoHTTP == 200)
  {
    resposta = http.getString(); //* Retorna o conteudo HTML da pagina
  }
  else
  {
    Serial.printf("Deu erro. Codigo: %d\n\r", codigoHTTP);
  }
  http.end();
  return resposta;
}


bool montarTelalcd(int id) {
    String jsonString = pedirParaApi(id);
    if (jsonString.length() == 0) return false;

    // Filtra apenas o necessário para montar a tela completa
    JsonDocument filter;
    filter["name"] = true;
    filter["id"] = true;
    filter["types"][0]["slot"] = true;
    filter["types"][0]["type"]["name"] = true;

    JsonDocument doc;
    DeserializationError erroJson = deserializeJson(doc, jsonString, DeserializationOption::Filter(filter));

    if (erroJson) {
        Serial.printf("Erro ao interpretar JSON: %s\n", erroJson.c_str());
        return false;
    }

    typesPokemon[0] = "";
    typesPokemon[1] = "";

    if (doc["name"].is<const char*>()) {
        nomePokemon = doc["name"].as<String>();
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Pokemon: %-11s", nomePokemon.c_str());

    lcd.setCursor(0, 1);
    lcd.printf("Pokedex: %-11d", id);

    if (doc["types"].is<JsonArray>()) {
        JsonArray types = doc["types"];
        int qntTypes = types.size();

        for (int i = 0; i < qntTypes && i < 2; i++) {
            int slot = types[i]["slot"].as<int>();
            const char* typeName = types[i]["type"]["name"].as<const char*>();

            typesPokemon[i] = String(typeName);

            lcd.setCursor(0, i + 2);
            lcd.printf("Type %d: %-12s", slot, typeName);
        }
    }
    return true;
}


void mqttPokemon () {
    const char *usuario = "GustavoRosa";

    Serial.println("Formatando as mensagens de erro");
    JsonDocument doc;
    
    doc["usuario"] = usuario;
    doc["pokemon"] = nomePokemon;
    doc["pokedex"] = pokedex;
    doc["type1"] = typesPokemon[0];
    doc["type2"] = typesPokemon[1];

    String mensagem;
    serializeJson(doc, mensagem);

    mqtt.publish(TOPICO_POKEMON, mensagem.c_str());
    Serial.printf("Mensagem a ser enviada no topico %s: \n\r%s", TOPICO_POKEMON, mensagem.c_str());
}

void buscarNomePokemon(int id) {
    String jsonString = pedirParaApi(id);

    if (jsonString.length() == 0) {
        nomePokemon = "Nao encontrado";
        return;
    }

    // Filtro restrito: pega apenas a chave "name"
    JsonDocument filter;
    filter["name"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonString, DeserializationOption::Filter(filter));

    if (!err && doc["name"].is<const char*>()) {
        nomePokemon = doc["name"].as<String>();
    } else {
        nomePokemon = "Erro JSON";
    }
}