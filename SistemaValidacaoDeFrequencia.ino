/******************************************************************
- Vcc ) 3V3 (ou Vin(5V), dependendo da versão do módulo)
- D14 ) RST (Reset)
- GND ) GND (Masse)
- 19  ) MISO (Master Input Slave Output)
- 23  ) MOSI (Master Output Slave Input)
- 18  ) SCK (Serial Clock)
- 5   ) SS/SDA (Slave select)
******************************************************************/

//Adafruit.IO
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <SPIFFS.h>
#include <MFRC522.h>  // Biblioteca para o RFID

#define RST_PIN 14
#define SS_PIN 5
#define IO_USERNAME  "AndreGomes" //<----------Mudar para o seu
#define IO_KEY       "aio_IyCc72SRy2uM2hf3zSRT4ECALvnj" //<----------Mudar para o seu
const char* ssid = "Wokwi-GUEST";
const char* password ="";

const char* mqttserver = "io.adafruit.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;
int oldTotalFreq = 1;
int oldLostFreq = 1;
bool test = true;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

//Botão Verde
int btPin = 2;
int leitura = 0;

//Botão Vermelho
int btPin_2 = 4;
unsigned long lastDebounceTime = 0; // Tempo do último debounce
unsigned long debounceDelay = 50;  // Delay para debounce
bool lastButtonState = LOW; //Último "Estado" do Botão
bool currentButtonState = LOW; //Atual "Estado" do Botão

//Define as entradas do Sensor de distância ultrassônico HC-SR04
#define PIN_TRIG 12
#define PIN_ECHO 14

//Define os LEDs
#define Green 33
#define Red 25
#define Blue 32

//RFID
#define RST_PIN 14
#define SS_PIN 5

//Dados da Frequência
int freq1 = 0;
int freq2 = 0;
int rFreq = 10;

int tFreq = 0; //Tempo de Aula decorrido
int tAula = 240; //Tempo de Aula em segundos

int oldSec = 0; //Anti-bounce para o Tempo de Aula
int r = random(0, 40); //Valor Aleatório para a Frequência
bool pausado = false; //Indica se está pausado

//Contadores e Verificadores
unsigned long cont_tFreq = 0; //Tempo total
unsigned long tempoSalvo = 0; //Tempo salvo ao pausar
int c = 0; //Contador para a Finalização da Aula

bool pausaPorDistancia = false; //Verificador de Pausa por Distância
bool autoFreq = false;
int oldPausado = 1;
int totalFreq = 0;
int lostFreq = 0;

int intervaloFreq = 30;
int defineFreq = 4;
int atualFreq = 0;

MFRC522 rfid(SS_PIN, RST_PIN);

// Variáveis para SPIFFS
String filename = "/config.txt";

// Função para salvar o estado no SPIFFS
void saveState(int tFreq, int tAula, unsigned long tempoSalvo, int sLost, int sFreq) {
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (file) {
    file.println(String(tFreq));
    file.println(String(tAula));
    file.println(String(tempoSalvo));
    file.println(String(sLost));
    file.println(String(sFreq));
    file.close();
    Serial.println("Estado salvo no SPIFFS.");
  } else {
    Serial.println("Erro ao salvar o estado no SPIFFS.");
  }
}

// Função para carregar o estado do SPIFFS
void loadState() {
  File file = SPIFFS.open(filename, FILE_READ);
  if (file) {
    tFreq = file.readStringUntil('\n').toInt();
    tAula = file.readStringUntil('\n').toInt();
    tempoSalvo = file.readStringUntil('\n').toULong();
    sLost = file.readStringUntil('\n').toInt();
    sFreq = file.readStringUntil('\n').toInt();
    file.close();
    Serial.println("Estado carregado do SPIFFS.");
  } else {
    Serial.println("Arquivo de estado não encontrado. Iniciando com valores padrão.");
    tFreq = 0;
    tAula = 240;
    tempoSalvo = 0;
    sLost = 0;
    sFreq = 0;
  }
}

//Adafruit.IO - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
const char* feedTfreq = "AndreGomes/feeds/tFreq"; // Nome do feed tFreq

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  
  String messageTemp;

  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  Serial.println(messageTemp);

  // Verifica se a mensagem pertence ao feed tFreq
  if (String(topic) == feedTfreq) {
    tFreq = messageTemp.toInt(); // Converte a mensagem para inteiro e armazena em tFreq
    Serial.print("tFreq atualizado: ");
    Serial.println(tFreq);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    String clientId = "ESP32-Sensores-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Conectado ao MQTT");

      // Publica uma mensagem inicial
      client.publish("AndreGomes/feeds/time-ui", "Iniciando Comunicação");
      client.publish("AndreGomes/feeds/userinwork", "Iniciando Comunicação");
      client.publish("AndreGomes/feeds/lostfreq", "Iniciando Comunicação");
      client.publish("AndreGomes/feeds/rFreq", "Iniciando Comunicação");

      // Faz o subscribe ao feed tFreq
      client.subscribe(feedTfreq);
      Serial.println("Inscrito no feed tFreq");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}
//Adafruit.IO - - - - - - - - - - - - - - - - - - - - - - - - - - - - 


//Formatar o Tempo
String formatTime(unsigned long time) {
  int minutes = time / 60000;
  int seconds = (time % 60000) / 1000;
  char buffer[12];
  sprintf(buffer, "%02d:%02d", minutes, seconds);
  return String(buffer);
}

//Retirar o Delay
int formatSeconds(unsigned long time) {
  int minutes = time / 60000;
  int seconds = (time % 60000) / 1000;
  return seconds;
}

//Led RGB
void corLed(int R, int G, int B) {
    digitalWrite(Red, R);
    digitalWrite(Green, G);
    digitalWrite(Blue, B);
}

void setup() {
  Serial.begin(9600);
  pinMode(btPin, INPUT);
  pinMode(btPin_2, INPUT);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(Red, OUTPUT);
  pinMode(Blue, OUTPUT);
  pinMode(Green, OUTPUT);
  
  setup_wifi();
  client.setServer(mqttserver, 1883); // Publicar
  client.setCallback(callback); // Receber mensagem 
  
  corLed(0, 254, 0);
    // Inicializa SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Falha ao montar o sistema de arquivos SPIFFS.");
    return;
  }

  // Carrega estado salvo
  loadState();

  // Inicializa WiFi
  setup_wifi();
  client.setServer(mqttserver, 1883);
  client.setCallback(callback);

  // Inicializa RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID inicializado. Aguardando conexão...");
}

void loop() {
  
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    Serial.println("Aguardando conexão do módulo RFID...");
    delay(1000);
    return;
  }

  if (!client.connected()) {
    reconnect();
  }
  
  while (rFreq == 0){
    client.loop();
  }
  
  //Leitura do Sensor
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  //Distância
  int duration = pulseIn(PIN_ECHO, HIGH);
  int cm = duration * 0.034 / 2;
  bool leitura_2 = digitalRead(btPin_2); //Leitura do Botão Vermelho

  //Pausar por DISTÂNCIA
  if (cm <= 10 || cm >= 40) {
    if (!pausado && !pausaPorDistancia) {
      tempoSalvo = millis();
      pausado = true;
      pausaPorDistancia = true;
      Serial.println("\r\nTempo de Aula Pausado! Aproxime-se do sensor.");
      corLed(0, 254, 0);

    }
  } else if (pausaPorDistancia && pausado) {
    cont_tFreq += millis() - tempoSalvo;
    pausado = false;
    pausaPorDistancia = false;
    corLed(0, 0, 0);
    Serial.println("\r\nTempo de Aula Retomado por DISTÂNCIA!");
  }
  
  //Pausar por BOTÃO
  else if (cm > 10 || cm < 40){
    if (leitura_2 != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (leitura_2 != currentButtonState) {
        currentButtonState = leitura_2;

        if (currentButtonState == HIGH) {
          pausado = !pausado;
          if (pausado) {
            tempoSalvo = millis();
            corLed(254, 254, 0);
            Serial.println("\r\nTempo de Aula Pausado!");
          } else {
            cont_tFreq += millis() - tempoSalvo;
            corLed(0, 254, 0);
            Serial.println("\r\nTempo de Aula Retomado!");
          }
        }
      }
    }
    lastButtonState = leitura_2;

  }

  if (autoFreq == true && defineFreq != atualFreq){
    rFreq += intervaloFreq;
    autoFreq = false;
  } 

  if (!pausado) {
    tFreq = (millis() - cont_tFreq) / 1000; // Tempo em segundos
    leitura = digitalRead(btPin);

    //1ª Frequência
    if (tFreq > (rFreq + r) && tFreq <= ((rFreq + r) + 10)) {
      int resto = ((rFreq + r) + 10) - tFreq;

      if (leitura == LOW && resto != tFreq && resto != 0) {
        Serial.print("\r\nAperte o Botão Verde para validar a 1ª Frequência! Tempo restante: ");
        Serial.println(resto);
        corLed(254, 0, 254);
        delay(1000);

      } else if (leitura == LOW && resto == 0) {
        Serial.print("\r\nTempo Esgotado! A 1ª Frequência não foi validada.\r\n");
        corLed(254, 0, 0);
        delay(1000);
        corLed(0, 254, 0);
        autoFreq = true;
        atualFreq++;

      } else if (leitura == HIGH) {
        Serial.print("\r\nA 1ª Frequência foi validada com sucesso!\r\n");
        corLed(0, 254, 0);
        delay(1000);
        corLed(0, 254, 0);
        autoFreq = true;
        atualFreq++;

      } else {
        int sec = formatSeconds(millis());
        if (sec != oldSec){
          Serial.print("\r\nTempo de Aula: ");
          Serial.println(formatTime(millis() - cont_tFreq));
          oldSec = sec;
        }
      }
    }

    //Finalizar aula
    else if (tFreq >= tAula) {
      if (c < 1) {
        c += 1;
        Serial.print("\r\nAula Finalizada!");
        Serial.print("\r\nTotal de Frequências Cadastradas: ");
        Serial.println(freq1 + freq2);
        delay(1000);

      }
    } else {
      int sec = formatSeconds(millis());
      if (sec != oldSec){
        Serial.print("\r\nTempo de Aula: ");
        Serial.println(formatTime(millis() - cont_tFreq));
        oldSec = sec;
      }
      
    }
  }

    if (pausado != oldPausado){
      char sUser[8];
      dtostrf(pausado,1,2,sUser);
      client.publish("AndreGomes/feeds/userinwork", sUser);
      oldPausado = pausado;
    }

    if (oldTotalFreq != totalFreq){
      char sFreq[8];
      dtostrf(totalFreq,1,2,sFreq);
      client.publish("AndreGomes/feeds/time-ui", sFreq);
      oldTotalFreq = totalFreq;
    }

    if (oldLostFreq != lostFreq){
      char sLost[8];
      dtostrf(lostFreq,1,2,sLost);
      client.publish("AndreGomes/feeds/lostfreq", sLost);
      oldLostFreq = lostFreq;
    }
  saveState(tFreq, tAula, tempoSalvo, sLost, sFreq);
}
