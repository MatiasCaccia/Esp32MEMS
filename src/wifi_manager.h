#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/**
 * Enfoque Header Only
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

class Publicador_v1 {
public:
    Publicador_v1(const char* server, int port, const char* endpoint) : server(server), port(port), endpoint(endpoint) {}

    void setCredentials(const char* username, const char* password) {
        this->username = username;
        this->password = password;
    }

    bool enviarDatos(const String& datos) {
        WiFiClient client;
        HTTPClient http;
        String url = "http://" + String(server) + ":" + String(port) + String(endpoint);

        http.begin(client, url);

        if (username != "" && password != "") {
            http.setAuthorization(username.c_str(), password.c_str());
        }

        int httpResponseCode = http.POST(datos);

        if (httpResponseCode > 0) {
            Serial.printf("[HTTP] POST respuesta código: %d\n", httpResponseCode);
            String response = http.getString();
            Serial.println(response);
        } else {
            Serial.printf("[HTTP] POST error código: %d\n", httpResponseCode);
        }

        http.end();

        return httpResponseCode > 0;
    }

private:
    const char* server;
    int port;
    const char* endpoint;
    String username;
    String password;
};

class WiFiManager {
public:
    
    WiFiManager() {
        // Constructor
        // Se pueden inicializar variables miembro aquí
    }
    
    // Método para establecer el SSID
    void setSSID(const char* ssid) {
        this->ssid = ssid;
    }

    // Método para establecer la contraseña
    void setPassword(const char* password) {
        this->password = password;
    }
    
    // Funciones para la gestión de la conexión a Internet
    void connectToWiFi(const char* ssid, const char* password) {
        WiFi.begin(ssid, password);
        // Se puede agregar la espera a la conexión
    }

    bool isWiFiConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    // Función de la tarea FreeRTOS para la gestión de la conexión
    static void taskFunction(void* parameter) {
        WiFiManager* wifiManager = static_cast<WiFiManager*>(parameter);
        
        /*
        WiFiClient espClient;
        PubSubClient client(espClient);

        const char* mqtt_server = "192.168.0.61";
        const int mqtt_port = 1883;
        const char* mqtt_topic = "Sensor";
        
        client.setServer(mqtt_server, mqtt_port);
        */
        PubSubClient client(WiFiClient);
        
        while (1) {
            if (!wifiManager->isWiFiConnected()) {
                Serial.println("Intentando conectar");
                wifiManager->connectToWiFi(wifiManager->ssid, wifiManager->password);
            } else{
                Serial.print("!");
                /*
                if (client.connected()) {
                    client.publish(wifiManager->mqtt_topic, "Hola, ¿estás?");
                }else{
                    Serial.print("No MQTT");
                    assert(false);
                }
                */
            }
            // Puedes agregar otras lógicas aquí
            // Publicar en MQTT si está conectado
            

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    /**
     * Falta el manejo de errores
    */

private:
    const char* ssid;      // Nueva variable miembro para el SSID
    const char* password;  // Nueva variable miembro para la contraseña
    
    const char* mqtt_server = "192.168.0.61";
    const int mqtt_port = 1883;
    const char* mqtt_topic = "Sensor";
    //WiFiClient espClient;
    //PubSubClient client(WiFiClient);

};

#endif // WIFI_MANAGER_H
