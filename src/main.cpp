#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Botao.h>
#include <HTTPClient.h>

void aoReceber(char*, byte*, unsigned int);
void tratarJson(String);
void montarTelaLcd();
String pedirParaApi(String);
void statusConexao();

LiquidCrystal_I2C lcd (0x27,20,4);
Botao btnCima(13);
Botao btnBaixo(14);
Botao btnEnter(15);


const char* SSID = "SENAI EDUCACIONAL";
const char* SENHA = "info@134";

const String enderecoAPI = "https://pokeapi.co/api/v2/pokemon/";

const char* BROKER = "broker.emqx.io"; //* endereço do servidor mqtt
const int PORTA = 1883;
const char* TOPICO_SUB = "senai134/esp32/eduardo-gustavo/comando";
const char* TOPICO_PUB = "senai134/esp32/eduardo-gustavo/status";

WiFiClient wifiClient; //* Instancia um cliente para que outros objetos utilizem a conexão WiFi
PubSubClient mqtt(wifiClient); //* Instancia um objeto para criar conexão com o broker
int contador = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  lcd.init();
  lcd.backlight();

  btnCima.iniciar();
  btnEnter.iniciar();
  btnBaixo.iniciar();
  

  Serial.printf("Conectando na rede %s\n\r", SSID);
  rgbLedWrite(48, 255, 0, 0);
  WiFi.begin(SSID, SENHA);

  while(WiFi.status() != WL_CONNECTED) { 
    Serial.print(".");
    rgbLedWrite(48, 0, 0, 0);
    delay(500);
    rgbLedWrite(48, 255, 255, 0);
    delay(200);
  }
  delay(1000);

  Serial.printf("\n\rConectado a rede %s com sucesso\n\r", SSID);

  rgbLedWrite(48, 255, 255, 0);

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
    rgbLedWrite(48, 0, 255, 0);
  } else {
    Serial.printf("falha ao conectar ao broker %s\n\r", BROKER); //* Adicionado BROKER que faltava
    Serial.print("Falha: "); 
    Serial.println(mqtt.state());
    rgbLedWrite(48, 255, 0, 0);
    delay(1000);
    rgbLedWrite(48,255,255,0);
  }
}

void loop() { 
  mqtt.loop(); 
  statusConexao();

  String pokemon = pedirParaApi(String(contador));
  static int flagEnvio = 0;

  btnBaixo.atualizar();
  btnCima.atualizar();
  btnEnter.atualizar();

  if(btnBaixo.pressionou()){
    if(contador < 1025) contador++;
  }
  if(btnCima.pressionou()){
    if(contador > 0 ) contador--;
  }
  if(btnEnter.pressionou()){
   montarTelalcd(pokemon);
  }
}

void aoReceber(char* topico, byte* payload, unsigned int tamanho){
  String mensagem;
  mensagem.reserve(tamanho); //* evita realocações enquanto a string cresce.

  for(unsigned int i = 0; i < tamanho; i++) { mensagem += (char)payload[i]; }

  Serial.printf("Mensagem Recebida no tópico: %s \n\r mensagem:\n\r %s\n\n\r", topico, mensagem.c_str());

  rgbLedWrite(48, 0, 0, 0);
  delay(500);
  rgbLedWrite(48, 0, 0, 255);
  delay(200);

  tratarJson(mensagem);
}

void tratarJson(String mensagem){
  JsonDocument doc;

  DeserializationError codigoErro = deserializeJson(doc, mensagem);

  if (codigoErro) {
    Serial.print("Falha na desserializacao: ");
    Serial.println(codigoErro.f_str());
    rgbLedWrite(48,255,0,0);
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

String pedirParaApi(String codigo){
    HTTPClient http;
    http.begin(enderecoAPI + codigo);

    int codigoHTTP = http.GET(); //* Retorna o codigo HTTP ex.:200 -> OK ou 404 -> Not found

    String resposta;

    if(codigoHTTP == 200){
        resposta = http.getString();//* Retorna o conteudo HTML da pagina
    }
    else {
        Serial.printf("Deu erro. Codigo: %d\n\r",codigoHTTP );
    }
    http.end();
    return resposta;
}

void montarTelalcd(String pokemon){

  JsonDocument doc;
  DeserializationError erroJson = deserializeJson(doc, pokemon );

   switch(erroJson.code()){
    case DeserializationError :: Ok: break;;
    case DeserializationError::InvalidInput: Serial.println("Json mal formado"); return;
    case DeserializationError :: EmptyInput : Serial.println("JSON vazio ou timeout"); return;
    case DeserializationError::IncompleteInput: Serial.println("JSON incompleto"); return;
    case DeserializationError :: NoMemory : Serial.println("Sem memoriat"); return;
    case DeserializationError:: TooDeep : Serial.println("Alinhamento acima do limite definido pela funcao"); return;

    default: Serial.println(erroJson.c_str()); break;
 
  }

  const char* nome = "";
  int pokedex = 0;
  

  if(doc["name"].is<const char*>())
  {
     nome = doc["name"].as<const char*>();
     Serial.printf("Pokemon: %s", nome);
     lcd.setCursor(0,0);
     lcd.printf("Pokemon: %s", nome);
  }
  if(doc["id"].is<int>())
  {
     pokedex = doc["id"].as<int>();
     Serial.printf("Pokedex: %u", pokedex);
     lcd.setCursor(0,1);
     lcd.printf("Pokedex: %u", pokedex);
  }
  if(doc["types"].is<JsonArray>()){
    JsonArray types = doc["types"];
    int qntTypes = types.size();

    for(int i = 0; i < qntTypes ;i++){

    int slot = types[i]["slot"].as<int>();

    Serial.printf("Type %u: %s",slot,types[i]["type"]["name"].as<const char*>());

    lcd.setCursor(0,i+2);
    lcd.printf("Type %u: %s",slot,types[i]["type"]["name"].as<const char*>());
    }

  }
  }

  void statusConexao(){
  const unsigned long tempoEntreEnvios = 5000;
  static unsigned long ultimoEnvio = 0;
  bool conectado = mqtt.state() == MQTT_CONNECTED ? true : false;
  
  const char* usuario = "GustavoRosa";
  IPAddress IP = WiFi.localIP();
  unsigned int timestamp = 1793796662;
  String mensagemErro = "Sem erro";


  if(millis() - ultimoEnvio > tempoEntreEnvios){

    Serial.println("Formatando as mensagens de erro");

    ultimoEnvio = millis();
    JsonDocument doc;
    doc["conectado"] = conectado;
    doc["usuario"] = usuario;
    doc["IP"] = IP.toString();
    doc["timestamp"] = timestamp;
    doc["mensagemErro"] = mensagemErro;


    String mensagem;
    serializeJson(doc,mensagem);

    mqtt.publish(TOPICO_PUB, mensagem.c_str());
    Serial.printf("Mensagem a ser enviada no topico %s: \n\r%s",TOPICO_PUB, mensagem.c_str());
  }
}