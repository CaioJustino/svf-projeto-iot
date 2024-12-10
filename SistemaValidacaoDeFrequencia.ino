//Adafruit.IO
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>

#define IO_USERNAME  "AndreGomes"
#define IO_KEY       "aio_TOWw08ZtS6JHEc7InqHDqwS71jEK"
const char* ssid = "NPITI-IoT";
const char* password ="NPITI-IoT";

const char* mqttserver = "io.adafruit.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;
int oldTotalFreq = 0;
int oldLostFreq = 0;
bool test = true;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


//SPIFFS

#include <FS.h>
#include <SPIFFS.h>


//Configuração do Projeto

//Botão Verde
int btPin = 5;
int leitura = 0;

//Botão Vermelho
int btPin_2 = 4;
unsigned long lastDebounceTime = 0; //Tempo do último debounce
unsigned long debounceDelay = 50;  //Delay para debounce
bool lastButtonState = LOW; //Último "Estado" do Botão
bool currentButtonState = LOW; //Atual "Estado" do Botão

//Define as entradas do Sensor de distância ultrassônico HC-SR04
#define PIN_TRIG 33
#define PIN_ECHO 32

//Define os LEDs
#define GREEN 25
#define YELL 26
#define RED 27

//Dados da Frequência
int freq1 = 0;
int freq2 = 0;
int totalFreq = 0;
int lostFreq = 0;
int validaFreq = lostFreq + totalFreq;
int tFreq = 0; //Tempo de Aula decorrido
int tAula = 240; //Tempo de Aula em segundos

int oldSec = 0; //Anti-bounce para o Tempo de Aula
int r = random(0, 40); //Valor Aleatório para a Frequência
bool pausado = false; //Indica se está pausado
bool oldPausado = false;

//Contadores e Verificadores
unsigned long cont_tFreq = 0; //Tempo total
unsigned long tempoSalvo = 0; //Tempo salvo ao pausar
int c = 0; //Contador para a Finalização da Aula

bool pausaPorDistancia = false; //Verificador de Pausa por Distância


//Adafruit.IO - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
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

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Messagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    //messageTemp += (char)payload[i]; <----------Usar quando tiver uma mensagem na resposta do bloco
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    //Cria um client ID aleatório
    String clientId = "ESP32 - Sensores";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("conectado");
      
      client.publish("AndreGomes/feeds/time-ui", "Iniciando Comunicação");
      client.publish("AndreGomes/feeds/userinwork", "Iniciando Comunicação");
      client.publish("AndreGomes/feeds/lostfreq", "Iniciando Comunicação");

    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}
//Adafruit.IO - - - - - - - - - - - - - - - - - - - - - - - - - - - - 


//ISppifsTime - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void writeFile(String state, String path) {
  File rFile = SPIFFS.open(path, "w");
  if (!rFile) {
    Serial.println("Erro ao abrir arquivo para escrita!");
    return;
  }
  if (rFile.print(state)) {
    Serial.print("Gravou: ");
    Serial.println(state);
  } else {
    Serial.println("Erro ao gravar no arquivo!");
  }
  rFile.close();
}

//Função que lê o código já gravado no ESP no endereço especificado
String readFile(String path) {
  File rFile = SPIFFS.open(path, "r");
  if (rFile) {
    String s = rFile.readStringUntil('\n');
    s.trim();
    Serial.print("Lido: ");
    Serial.println(s);
    rFile.close();
    return s;
  } else {
    Serial.println("Erro ao abrir arquivo!");
    return "";
  }
}

//Função que inicia o protocolo
void openFS() {
  if (SPIFFS.begin()) {
    Serial.println("Sistema de arquivos aberto com sucesso!");
  } else {
    Serial.println("Erro ao abrir o sistema de arquivos");
  }
}

String strSppifsTime, readSSppifsTime;
unsigned long sppifsTime = 0;
unsigned long millisAdjustedTIME = 5000;

//ISppifsTime - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 


//Função para Formatar o Tempo
String formatTime(unsigned long time) {
  int minutes = time / 60000;
  int seconds = (time % 60000) / 1000;
  char buffer[12];
  sprintf(buffer, "%02d:%02d", minutes, seconds);
  return String(buffer);
}

//Função para Retirar o Delay
int formatSeconds(unsigned long time) {
  int minutes = time / 60000;
  int seconds = (time % 60000) / 1000;
  return seconds;
}

void setup() {
  openFS();

  Serial.begin(9600);
  pinMode(btPin, INPUT);
  pinMode(btPin_2, INPUT);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  pinMode(GREEN, OUTPUT);
  pinMode(YELL, OUTPUT);
  pinMode(RED, OUTPUT); 

  setup_wifi();
  client.setServer(mqttserver, 1883); //Publicar
  client.setCallback(callback); //Receber mensagem

  //Entrada
  String savedTotalFreq = readFile("/totalFreq.txt");
  if (savedTotalFreq != "") {
    totalFreq = savedTotalFreq.toInt();
    Serial.print("Total Frequências carregadas: ");
    Serial.println(totalFreq);
  }

  String savedLostFreq = readFile("/lostFreq.txt");
  if (savedLostFreq != "") {
    lostFreq = savedLostFreq.toInt();
    Serial.print("Frequências Perdidas carregadas: ");
    Serial.println(lostFreq);
  }

  //Ler o Tempo Salvo
  if (readFile("/logstTime.txt") != "") {
    readSSppifsTime = readFile("/logstTime.txt");
    sppifsTime = readSSppifsTime.toInt();

    if (sppifsTime == 0 && readSSppifsTime != "0") {
      sppifsTime = 0;
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  //Calcular o tempo ajustado
  unsigned long millisAdjusted = millis() + sppifsTime;

  if (millisAdjusted == millisAdjustedTIME ){
    strSppifsTime = String(millisAdjusted);
    writeFile(strSppifsTime, "/logstTime.txt");
    millisAdjustedTIME += 10000;
  }
  
  //Leitura do sensor
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  //Distância
  int duration = pulseIn(PIN_ECHO, HIGH);
  int cm = duration * 0.034 / 2;
  bool leitura_2 = digitalRead(btPin_2); //Leitura do botão vermelho
  if (leitura_2 == true) {
    leitura_2 = false;
  } else {
    leitura_2 = true;
  }

  //Pausar por distância
  if (cm <= 10 || cm >= 40) {
    if (!pausado && !pausaPorDistancia) {
      tempoSalvo = millisAdjusted;
      pausado = true;
      pausaPorDistancia = true;
      Serial.println("\r\nTempo de Aula Pausado! Aproxime-se do sensor.");
      digitalWrite(YELL, HIGH);
    }
  } else if (pausaPorDistancia && pausado) {
    cont_tFreq += millisAdjusted - tempoSalvo;
    pausado = false;
    pausaPorDistancia = false;
    digitalWrite(YELL, LOW);
    Serial.println("\r\nTempo de Aula Retomado!");
  }

  //Pausar por botão
  else if (cm > 10 || cm < 40) {
    if (leitura_2 != lastButtonState) {
      lastDebounceTime = millisAdjusted;
    }

    if ((millisAdjusted - lastDebounceTime) > debounceDelay) {
      if (leitura_2 != currentButtonState) {
        currentButtonState = leitura_2;

        if (currentButtonState == HIGH) {
          pausado = !pausado;
          if (pausado) {
            tempoSalvo = millisAdjusted;
            digitalWrite(YELL, HIGH);
            Serial.println("\r\nTempo de Aula Pausado!");
          } else {
            cont_tFreq += millisAdjusted - tempoSalvo;
            digitalWrite(YELL, LOW);
            Serial.println("\r\nTempo de Aula Retomado!");
          }
        }
      }
    }
    lastButtonState = leitura_2;
  }

  //Processos enquanto não está pausado
  if (!pausado) {
    tFreq = (millisAdjusted - cont_tFreq) / 1000; // Tempo em segundos
    leitura = digitalRead(btPin);

    if (leitura == true) {
      leitura = false;
    } else {
      leitura = true;
    }

    //1ª Frequência
    if (tFreq > (10 + r) && tFreq <= ((10 + r) + 10) && validaFreq == 0) {
      int resto = ((10 + r) + 10) - tFreq;

      if (leitura == LOW && resto != tFreq && resto != 0 && freq1 == 0) {
        Serial.print("\r\nAperte o Botão Verde para validar a 1ª Frequência! Tempo restante: ");
        Serial.println(resto);
        digitalWrite(YELL, HIGH);
        delay(1000);
        digitalWrite(YELL, LOW);

      } else if (leitura == LOW && resto == 0 && freq1 == 0) {
        lostFreq++;
        Serial.print("\r\nTempo Esgotado! A 1ª Frequência não foi validada.\r\n");
        digitalWrite(RED, HIGH);
        delay(1000);
        digitalWrite(RED, LOW);

      } else if (leitura == HIGH && freq1 == 0) {
        freq1 = 1;
        totalFreq++;
        Serial.print("\r\nA 1ª Frequência foi validada com sucesso!\r\n");
        digitalWrite(GREEN, HIGH);
        delay(1000);
        digitalWrite(GREEN, LOW);

      } else {
        int sec = formatSeconds(millisAdjusted);
        if (sec != oldSec) {
          Serial.print("\r\nTempo de Aula: ");
          Serial.println(formatTime(millisAdjusted - cont_tFreq));
          oldSec = sec;
        }
      }
    }

    //2ª Frequência
    else if (tFreq > (70 + r) && tFreq <= ((70 + r) + 10) && validaFreq == 1) {
      int resto = ((70 + r) + 10) - tFreq;

      if (leitura == LOW && resto != tFreq && resto != 0 && freq2 == 0) {
        Serial.print("\r\nAperte o Botão Verde para validar a 2ª Frequência! Tempo restante: ");
        Serial.println(resto);
        digitalWrite(YELL, HIGH);
        delay(50);
        digitalWrite(YELL, LOW);
        delay(1000);

      } else if (leitura == LOW && resto == 0 && freq2 == 0) {
        lostFreq++;
        Serial.print("\r\nTempo Esgotado! A 2ª Frequência não foi validada.\r\n");
        digitalWrite(RED, HIGH);
        delay(50);
        digitalWrite(RED, LOW);
        delay(1000);

      } else if (leitura == HIGH && freq2 == 0) {
        freq2 = 1;
        totalFreq++;
        Serial.print("\r\nA 2ª Frequência foi validada com sucesso!\r\n");
        digitalWrite(GREEN, HIGH);
        delay(50);
        digitalWrite(GREEN, LOW);
        delay(1000);

      } else {
        int sec = formatSeconds(millisAdjusted);
        if (sec != oldSec) {
          Serial.print("\r\nTempo de Aula: ");
          Serial.println(formatTime(millisAdjusted - cont_tFreq));
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
      int sec = formatSeconds(millisAdjusted);
      if (sec != oldSec) {
        Serial.print("\r\nTempo de Aula: ");
        Serial.println(formatTime(millisAdjusted - cont_tFreq));
        oldSec = sec;
      }
    }
  }

  //Publicar mensagens no MQTT
  if (test) {
    char sUser[8];
    dtostrf(pausado, 1, 2, sUser);
    client.publish("AndreGomes/feeds/userinwork", sUser);
    char sFreq[8];
    dtostrf(totalFreq, 1, 2, sFreq);
    client.publish("AndreGomes/feeds/time-ui", sFreq);
    char sLost[8];
    dtostrf(lostFreq, 1, 2, sLost);
    client.publish("AndreGomes/feeds/lostfreq", sLost);

    test = !test;
  }

  if (pausado != oldPausado) {
    char sUser[8];
    dtostrf(pausado, 1, 2, sUser);
    client.publish("AndreGomes/feeds/userinwork", sUser);
    oldPausado = pausado;
    delay(1000);
  }

  if (oldTotalFreq != totalFreq) {
    //Atualizar o valor no SPIFFS
    writeFile(String(totalFreq), "/totalFreq.txt");

    //Publicar no MQTT
    char sFreq[8];
    dtostrf(totalFreq, 1, 2, sFreq);
    client.publish("AndreGomes/feeds/time-ui", sFreq);

    oldTotalFreq = totalFreq;
    delay(1000);
  }

  if (oldLostFreq != lostFreq) {
    //Atualizar o valor no SPIFFS
    writeFile(String(lostFreq), "/lostFreq.txt");

    //Publicar no MQTT
    char sLost[8];
    dtostrf(lostFreq, 1, 2, sLost);
    client.publish("AndreGomes/feeds/lostfreq", sLost);

    oldLostFreq = lostFreq;
    delay(1000);
  }
}