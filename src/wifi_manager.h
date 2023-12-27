#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/**
 * Enfoque Header Only
*/

#include <Arduino.h>
#include <WiFi.h>

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

        while (1) {
            if (!wifiManager->isWiFiConnected()) {
                Serial.println("Intentando conectar");
                wifiManager->connectToWiFi(wifiManager->ssid, wifiManager->password);
            } else{
                Serial.print("!");
            }
            // Puedes agregar otras lógicas aquí

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    /**
     * Falta el manejo de errores
    */

private:
    const char* ssid;      // Nueva variable miembro para el SSID
    const char* password;  // Nueva variable miembro para la contraseña
};

#endif // WIFI_MANAGER_H
