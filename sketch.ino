// Autor: Fábio Henrique Cabrini
// Resumo: Este programa permite ligar e desligar o LED onboard, além de enviar o status para o Broker MQTT para que o Helix saiba se o LED está ligado ou desligado.
// Revisões:
// Rev1: 26-08-2023 Código portado para o ESP32 e para realizar a leitura de luminosidade e publicar o valor em um tópico apropriado do broker
// Autor Rev1: Lucas Demetrius Augusto
// Rev2: 28-08-2023 Ajustes para o funcionamento no FIWARE Descomplicado
// Autor Rev2: Fábio Henrique Cabrini

#include <WiFi.h>
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient

// Defines:
// Defines de ID MQTT e tópicos para publicação e subscrição denominados TEF (Telemetria e Monitoramento de Equipamentos)
#define TOPICO_SUBSCRIBE "/TEF/lamp108/cmd"       // Tópico MQTT de escuta
#define TOPICO_PUBLISH "/TEF/lamp108/attrs"       // Tópico MQTT de envio de informações para Broker
#define TOPICO_PUBLISH_2 "/TEF/lamp108/attrs/l"   // Tópico MQTT de envio de informações para Broker
                                                // IMPORTANTE: recomendamos fortemente alterar os nomes desses tópicos. Caso contrário, há grandes chances de você controlar e monitorar o ESP32 de outra pessoa.
#define ID_MQTT "fiware_108" // ID MQTT (para identificação de sessão)
                            // IMPORTANTE: este deve ser único no broker (ou seja, se um cliente MQTT tentar entrar com o mesmo ID de outro já conectado ao broker, o broker irá fechar a conexão de um deles).
                            // O valor "n" precisa ser único!

#include "DHT.h"

#define DHTPIN 4     // Pino digital conectado ao sensor DHT
#define DHTTYPE DHT22 // Tipo do sensor DHT (DHT 22)

DHT dht(DHTPIN, DHTTYPE);

const char *SSID = "Wokwi-GUEST"; // SSID / nome da rede Wi-Fi que deseja se conectar
const char *PASSWORD = "";        // Senha da rede Wi-Fi que deseja se conectar

const char *BROKER_MQTT = "46.17.108.113"; // URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;                   // Porta do Broker MQTT

int D4 = 2;

WiFiClient espClient;
PubSubClient MQTT(espClient);

char EstadoSaida = '0';

void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi();
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);

void setup()
{
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    delay(5000);
    MQTT.publish(TOPICO_PUBLISH, "s|off");

    Serial.begin(9600);
    Serial.println(F("DHTxx test"));

    dht.begin();
}

void initSerial()
{
    Serial.begin(115200);
}

void initWiFi()
{
    delay(10);
    Serial.println("------Conexao Wi-Fi------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");

    reconectWiFi();
}

void initMQTT()
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    String msg;

    for (int i = 0; i < length; i++)
    {
        char c = (char)payload[i];
        msg += c;
    }

    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    if (msg.equals("lamp108@on|"))
    {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    if (msg.equals("lamp108@off|"))
    {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

void reconnectMQTT()
{
    while (!MQTT.connected())
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT))
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        }
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

void reconectWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    WiFi.begin(SSID, PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected())
        reconnectMQTT();

    reconectWiFi();
}

void EnviaEstadoOutputMQTT(void)
{
    if (EstadoSaida == '1')
    {
        MQTT.publish(TOPICO_PUBLISH, "s|on");
        Serial.println("- Led Ligado");
    }

    if (EstadoSaida == '0')
    {
        MQTT.publish(TOPICO_PUBLISH, "s|off");
        Serial.println("- Led Desligado");

    }

    Serial.println("- Estado do LED onboard enviado ao broker!");
    delay(1000);
}

void InitOutput(void)
{
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);

    boolean toggle = false;

    for (int i = 0; i <= 10; i++)
    {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }

    digitalWrite(D4, LOW);
}

void loop()
{
    const int potPin = 34;

    char msgBuffer[4];

    VerificaConexoesWiFIEMQTT();

    EnviaEstadoOutputMQTT();

    delay(2000);

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = dht.readTemperature(true);

    if (isnan(h) || isnan(t) || isnan(f))
    {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }

    if (t > 13) {
      Serial.println(F("Temperatura maior do que a deseja!"));
    }

    if (t < 3) {
      Serial.println(F("Temperatura menor do que a deseja!"));
    }

    if (h > 70) {
      Serial.println(F("Umidade maior do que a deseja!"));
    }

    float hif = dht.computeHeatIndex(f, h);
    float hic = dht.computeHeatIndex(t, h, false);

    Serial.print(F("Umidade: "));
    Serial.print(h);
    Serial.print(F("%  Temperatura: "));
    Serial.print(t);
    Serial.print(F("°C "));
    Serial.print(f);
    Serial.print(F("°F  Heat index: "));
    Serial.print(hic);
    Serial.print(F("°C "));
    Serial.print(hif);
    Serial.println(F("°F"));

    int sensorValue = analogRead(potPin);
    float voltage = sensorValue * (3.3 / 4096.0);
    float luminosity = map(sensorValue, 0, 4095, 0, 100);

    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.print("V - ");
    Serial.print("Luminosity: ");
    Serial.print(luminosity);
    Serial.println("%");
    
    dtostrf(luminosity, 4, 2, msgBuffer);
    MQTT.publish(TOPICO_PUBLISH_2, msgBuffer);

    MQTT.loop();
}
