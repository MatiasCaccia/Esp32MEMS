#ifndef PUBLICADOR_H
#define PUBLICADOR_H

#include <Arduino.h>
#include <stdint.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SD.h>

class Publicador{
    private:
        //double vel_ant = 0;
        bool Publicador_info = false;
        File backupFile;

    public:
        void SerialInfo(bool mode);
        void Wifi_init(const char* ssid, const char* password);
        void Mqtt_init(const char* mqtt_server, uint16_t mqtt_port);
        void Mqtt_Connect(const char *clientId, const char *mqtt_user, const char *mqtt_pass);
        void Mqtt_Publish(const char* topic, const char* payload);
        void Mqtt_Suscribe(const char* topic);
        void Mqtt_KeepAlive();
    
        bool Mqtt_IsConnected();
        
        // Nuevo método para guardar en la tarjeta SD
        void SaveToSD(const char* data);
        void setupSD();
        
        // Nuevo método para verificar la conexión WiFi y MQTT 
        bool IsConnectionAvailable();
};

#endif
