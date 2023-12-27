#ifndef PUBLICADOR_H
#define PUBLICADOR_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class Publicador {
public:
    Publicador(const char* server, int port, const char* endpoint) : server(server), port(port), endpoint(endpoint) {}

    void setCredentials(const char* username, const char* password) {
        this->username = username;
        this->password = password;
    }

    bool enviarDatos(const String& datos) {
        WiFiClient client;

        HTTPClient http;

        String url = "http://" + String(server) + ":" + String(port) + String(endpoint);

        // Inicia la comunicación HTTP
        http.begin(client, url);

        // Si se necesitan credenciales, se configuran
        if (username != "" && password != "") {
            http.setAuthorization(username.c_str(), password.c_str());
        }

        // Envia la solicitud POST
        int httpResponseCode = http.POST(datos);

        // Si la respuesta fue exitosa
        if (httpResponseCode > 0) {
            Serial.printf("[HTTP] POST respuesta código: %d\n", httpResponseCode);

            // Lee la respuesta
            String response = http.getString();
            Serial.println(response);
        } else {
            Serial.printf("[HTTP] POST error código: %d\n", httpResponseCode);
        }

        // Termina la comunicación HTTP
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

#endif  // PUBLICADOR_H
