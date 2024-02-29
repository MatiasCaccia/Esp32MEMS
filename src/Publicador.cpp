#include <Arduino.h>
#include <Publicador.h>
#include <PubSubClient.h> // Incluye la librería PubSubClient para la comunicación MQTT
#include "SD_funciones.h"
//

WiFiClient PublicadorWifiClient; // Crea un objeto WiFiClient para gestionar la conexión WiFi
PubSubClient PublicadorMQTT(PublicadorWifiClient); // Crea un objeto PubSubClient para gestionar la comunicación MQTT
SPIClass SPI_card(SPI);

// Función de devolución de llamada (callback) para procesar los mensajes MQTT recibidos
void Mqtt_Callback(char* topic, byte* payload, unsigned int length);

/**
 * Inicializa la conexión WiFi con los parámetros proporcionados.
 * 
 * @param ssid SSID de la red WiFi.
 * @param password Contraseña de la red WiFi.
*/
void Publicador::Wifi_init(const char* ssid, const char* password){
	
    if(this->Publicador_info){ // Verifica si se debe imprimir información (modo verbose)
        Serial.println();
	    Serial.print("Conectando a ssid: ");
	    Serial.println(ssid);
    }

	WiFi.begin(ssid, password); // Inicia la conexión WiFi con los datos proporcionados

	// Espera hasta que se establezca la conexión WiFi
    while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		if(this->Publicador_info) Serial.print(".");
	}

    if(this->Publicador_info){ // Imprime información si está habilitado el modo verbose
	    Serial.println("");
	    Serial.println("Conectado!!");
	    Serial.println("Dirección IP: ");
	    Serial.println(WiFi.localIP());
    }
    WiFi.setSleep(false); // Desactiva el modo de suspensión de WiFi
}

/**
 * Intenta conectar al servidor MQTT con los credenciales proporcionados.
 * 
 * @param clientId Identificador único del cliente MQTT.
 * @param mqtt_user Nombre de usuario para la conexión MQTT.
 * @param mqtt_pass Contraseña para la conexión MQTT.
 * */
void Publicador::Mqtt_Connect(const char *clientId, const char *mqtt_user, const char *mqtt_pass){
	while (!PublicadorMQTT.connected()) { // Repite hasta que se conecte al servidor MQTT
		// Intenta conectar al servidor MQTT con los credenciales proporcionados
		if (PublicadorMQTT.connect(clientId,mqtt_user,mqtt_pass)) {
				if(this->Publicador_info) Serial.println("Conectado al Broker!");
		}else {
			if(this->Publicador_info){   
                Serial.print("falló :( con error -> ");
			    Serial.print(PublicadorMQTT.state());
			    Serial.println(" Intentamos de nuevo en 5 segundos");
			    delay(5000);
            }
		}
	}
}

/**
 * Configura la conexión al servidor MQTT con la dirección y el puerto especificados.
 * 
 * @param mqtt_server Dirección IP o nombre de dominio del servidor MQTT.
 * @param mqtt_port Puerto del servidor MQTT.
 */
void Publicador::Mqtt_init(const char *mqtt_server, uint16_t mqtt_port){
    PublicadorMQTT.setServer(mqtt_server, mqtt_port); // Configura la dirección y el puerto del servidor MQTT
    PublicadorMQTT.setCallback([this] (char* topic, byte* payload, unsigned int length) { Mqtt_Callback(topic, payload, length); }); // Configura la función de devolución de llamada
}

/**
 * Verifica si está conectado al servidor MQTT.
 * @return true si está conectado, false en caso contrario.
 */
bool Publicador::Mqtt_IsConnected(){
    // Devuelve true si está conectado al servidor MQTT, false en caso contrario
    return PublicadorMQTT.connected();
}

/**
 * Publica un mensaje en el servidor MQTT con el tema y el contenido proporcionados.
 * @param topic Tema del mensaje MQTT.
 * @param payload Contenido del mensaje MQTT.
 */
void Publicador::Mqtt_Publish(const char* topic, const char* payload){
    PublicadorMQTT.publish(topic,payload);  // Publica un mensaje en el servidor MQTT con el tema y el contenido proporcionados
}

/**
 * Se suscribe a un tema en el servidor MQTT si está conectado.
 * @param topic Tema al que se suscribirá.
 */
void Publicador::Mqtt_Suscribe(const char* topic){
    if(this->Mqtt_IsConnected()){  // Publica un mensaje en el servidor MQTT con el tema y el contenido proporcionados
         PublicadorMQTT.subscribe(topic); // Se suscribe al tema especificado
         if(this->Publicador_info){
             Serial.print("Suscrito a: ");
             Serial.println(topic);
         }else{
             Serial.println("Subscripcion fallida, no se tiene conexion con el broker");
         }
    }
}

/**
 * Mantiene activa la conexión MQTT y procesa los mensajes entrantes.
 */
void Publicador::Mqtt_KeepAlive(){
    PublicadorMQTT.loop(); // Mantiene activa la conexión MQTT y procesa los mensajes entrantes
}

/**
 * Habilita o deshabilita la impresión de información (modo verbose).
 * @param mode true para habilitar, false para deshabilitar.
 */
void Publicador::SerialInfo(bool mode){
    this->Publicador_info = mode;  // Habilita o deshabilita la impresión de información (modo verbose)
}

/**
* Función para guardar la medición siempre que no haya conexión WiFi o al broker MQTT
* @param data es lo que se va a guardar en caso que no haya conexión
*
*/
void Publicador::SaveToSD(const char* data){
  if(SD.begin()){
    backupFile = SD.open("/backup.txt", FILE_APPEND);
    if(backupFile){
      backupFile.println(data);
      backupFile.close();
    } else{
      Serial.println("Error al intentar abrir archivo backup.txt");
    }
  } else{
    Serial.println("Error iniciando la tajeta SD");
  }
}

bool Publicador::IsConnectionAvailable(){
  return (WiFi.status() == WL_CONNECTED) && PublicadorMQTT.connected();
}
