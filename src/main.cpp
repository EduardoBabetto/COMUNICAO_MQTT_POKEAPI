#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

void aoReceber(char*, byte*, unsigned int);
void tratarJson(String);

const char* SSID = "SENAI EDUCACIONAL";
const char* SENHA = "info@134";

const char* BROKER = "broker.emqx.io"; //* endereço do servidor mqtt
const int PORTA = 1883;
const char* TOPICO_SUB = "senai134/esp32/eduardo/comando";
const char* TOPICO_PUB = "senai134/esp32/eduardo/status";

WiFiClient wifiClient; //* Instancia um cliente para que outros objetos utilizem a conexão WiFi
PubSubClient mqtt(wifiClient); //* Instancia um objeto para criar conexão com o broker

void setup() {
  Serial.begin(115200);
  Serial.println();

  Serial.printf("Conectando na rede %s\n\r", SSID);

  WiFi.begin(SSID, SENHA);

  while(WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); }
  delay(1000);

  Serial.printf("\n\rConectado a rede %s com sucesso\n\r", SSID);

  Serial.printf("Conectando ao Broker %s\n\r", BROKER);

  mqtt.setServer(BROKER, PORTA); //! PORTA: 1883 ou 8883
  mqtt.setCallback(aoReceber);

  String id = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);

  bool conectadoAoBroker = false;
  conectadoAoBroker = mqtt.connect(id.c_str());   

  if(conectadoAoBroker) {
    Serial.printf("Conectado ao broker %s com sucesso!\n\r", BROKER);
    mqtt.subscribe(TOPICO_SUB); //* Ajustado para se inscrever no tópico de comando
    Serial.printf("Inscrito no tópico: %s\n\r", TOPICO_SUB);
  } else {
    Serial.printf("falha ao conectar ao broker %s\n\r", BROKER); //* Adicionado BROKER que faltava
    Serial.print("Falha: "); 
    Serial.println(mqtt.state());
  }
}

void loop() { 
  mqtt.loop(); 
  
  float temperatura = 50.0f;
  bool estado = true;
  unsigned int timestamp = 1793796662;
  const unsigned long tempoEntreEnvios = 5000;
  static unsigned long ultimoEnvio = 0;
  int posicaoDisplay = 3;
  String mensagemErro = "Sem erro";

  if(millis() - ultimoEnvio > tempoEntreEnvios){

    Serial.println("Formatando as mensagens de erro");

    ultimoEnvio = millis();
    JsonDocument doc;
    doc["temperatura"] = temperatura;
    doc["estado"] = estado;
    doc["timestamp"] = timestamp;
    doc["posDisplay"] = posicaoDisplay;
    doc["mensagemErro"] = mensagemErro;


    String mensagem;
    serializeJson(doc,mensagem);

    mqtt.publish(TOPICO_PUB, mensagem.c_str());
    Serial.printf("Mensagem a ser enviada no topico %s: \n\r%s",TOPICO_PUB, mensagem.c_str());
  }
}

void aoReceber(char* topico, byte* payload, unsigned int tamanho){
  String mensagem;
  mensagem.reserve(tamanho); //* evita realocações enquanto a string cresce.

  for(unsigned int i = 0; i < tamanho; i++) { mensagem += (char)payload[i]; }

  Serial.printf("Mensagem Recebida no tópico: %s \n\r mensagem:\n\r %s\n\n\r", topico, mensagem.c_str());

  tratarJson(mensagem);
}

void tratarJson(String mensagem){
  JsonDocument doc;

  DeserializationError codigoErro = deserializeJson(doc, mensagem);

  if (codigoErro) {
    Serial.print("Falha na desserializacao: ");
    Serial.println(codigoErro.f_str());
    return;
  }

  bool estadoLed = doc["estadoLed"].as<bool>();

  if(estadoLed) {
    Serial.println("LED LIGADO");
    rgbLedWrite(48, 0, 0, 255);
  } else { //* Corrigido: era else(estadoLed), trocado para apenas else
    Serial.println("LED DESLIGADO");
    rgbLedWrite(48, 0, 0, 0);
  }            
}