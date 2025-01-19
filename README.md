# Sistema de Validação de Frequência (SVF) 2.0

Sistema de Validação de Frequência (SVF), um projeto para a disciplina de Introdução a Internet das Coisas (IIC). O seu objetivo é verificar a presença dos alunos nas mesas de trabalho durante as aulas. Nessa aplicação, desenvolvida em [C++](https://learn.microsoft.com/pt-br/cpp/cpp/?view=msvc-170), foram utilizados o [SPIFFS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/spiffs.html) ("SPI Flash File System", isto é, Sistema de Arquivo Flash de Interface Periférica Serial) e a [Adafruit IO](https://io.adafruit.com/) (plataforma de IoT).
<br><br>

## 📋 Pré-requisitos

A princípio, antes de executar o sistema, certifique-se de montar o [Circuito](https://wokwi.com/projects/416164682176137217) e de instalar alguma IDE apropriada para executar códigos no formato ".ino", como a [Arduino IDE](https://www.arduino.cc/en/software).
<br><br>

## 🔧 Instalação

1. Clone este repositório em sua máquina local.

```
git clone https://github.com/CaioJustino/svf-projeto-iot.git
```

2. Na sua IDE, certifique de instalar a biblioteca "PubSubClient" para lidar com as dependências da conexão com a Adafruit.IO.

```
Na "Arduino IDE", acesse: Ferramentas > Gerenciar bibliotecas > PubSubClient
```

3. Por fim, basta executar o código.

```
run
```
<br>

## 🛠️ Construído com

* [C++](https://learn.microsoft.com/pt-br/cpp/cpp/?view=msvc-170) - Linguagem de programação responsável pelo desenvolvimento da lógica interna do sistema.
* [Adafruit IO](https://io.adafruit.com/) - Plataforma de IoT.
<br><br>

## ✒️ Autores

* **Desenvolvimento** - [André Soares](https://github.com/OfAndreS)
* **Desenvolvimento** - [Caio Justino](https://github.com/CaioJustino)
<br><br>

## ❤ Agradecimentos

Agradecimentos especiais ao professor Leonardo Augusto e ao monitor Gabriel por todo o suporte prestado ao decorrer do desenvolvimento desse projeto.
<br><br>
