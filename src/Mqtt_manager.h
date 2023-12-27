#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include <Arduino.h>
#include <WiFi.h>

class MQTTManager{
public:
    MQTTManager(const char* mqttServer, int mqttPort) : mqttClient(espClient) {
        // Constructor
        mqttClient.setServer(mqttServer, mqttPort);
    }

    // Método para establecer las credenciales MQTT (usuario y contraseña)
    void setCredentials(const char* mqttUser, const char* mqttPassword) {
        this->mqttUser = mqttUser;
        this->mqttPassword = mqttPassword;
    }

    // Función para conectar al broker MQTT
    bool connectToMqtt() {
        if (!mqttClient.connected()) {
            Serial.print("Conectando al broker MQTT...");
            
            if (mqttClient.connect("ESP32Client", mqttUser, mqttPassword)) {
                Serial.println("conectado!");
                return true;
            } else {
                Serial.print("falló, estado de conexión: ");
                Serial.println(mqttClient.state());
                return false;
            }
        }

        return true; // Ya está conectado
    }
    // Función para publicar un mensaje en un tópico MQTT
void publish(const char* topic, const char* message) {
        if (mqttClient.connected()) {
            mqttClient.publish(topic, message);
        } else {
            Serial.println("No conectado a MQTT Broker. Intentando reconectar...");
            connectToMqtt();
        }
    }

    // Método para manejar eventos MQTT (debe ser llamado en el loop)
    void handleMqtt() {
        mqttClient.loop();
    }

private:
    WiFiClient espClient;
    PubSubClient mqttClient;

    const char* mqttUser;       // Variable miembro para el usuario MQTT
    const char* mqttPassword;   // Variable miembro para la contraseña MQTT


};

#endif