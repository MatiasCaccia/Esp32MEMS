#include <Arduino.h>
#include <Publicador.h>
#include <PubSubClient.h>

WiFiClient PublicadorWifiClient;
PubSubClient PublicadorMQTT(PublicadorWifiClient);

void Mqtt_Callback(char* topic, byte* payload, unsigned int length);

/*
void Publicador::init(int freq){
    ledcSetup(1, freq, 10);
    ledcSetup(2, freq, 10);
}
*/

/* Metodos reescritos para un mejor entendimiento */

void Publicador::Wifi_init(const char* ssid, const char* password){
	
    if(this->Publicador_info){
        Serial.println();
	    Serial.print("Conectando a ssid: ");
	    Serial.println(ssid);
    }

	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		if(this->Publicador_info) Serial.print(".");
	}

    if(this->Publicador_info){
	    Serial.println("");
	    Serial.println("Conectado!!");
	    Serial.println("Dirección IP: ");
	    Serial.println(WiFi.localIP());
    }
    WiFi.setSleep(false);
}

void Publicador::Mqtt_Connect(const char *clientId, const char *mqtt_user, const char *mqtt_pass){

	while (!PublicadorMQTT.connected()) {
		
		// Intentamos conectar
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

void Publicador::Mqtt_init(const char *mqtt_server, uint16_t mqtt_port){
    PublicadorMQTT.setServer(mqtt_server, mqtt_port);
    PublicadorMQTT.setCallback([this] (char* topic, byte* payload, unsigned int length) { Mqtt_Callback(topic, payload, length); });
}

bool Publicador::Mqtt_IsConnected(){
    return PublicadorMQTT.connected();
}

void Publicador::Mqtt_Publish(const char* topic, const char* payload){
    PublicadorMQTT.publish(topic,payload);
}

void Publicador::Mqtt_Suscribe(const char* topic){
    if(this->Mqtt_IsConnected()){
         PublicadorMQTT.subscribe(topic);
         if(this->Publicador_info){
             Serial.print("Suscrito a: ");
             Serial.println(topic);
         }else{
             Serial.println("Subscripcion fallida, no se tiene conexion con el broker");
         }
    }
}

void Publicador::Mqtt_KeepAlive(){
    PublicadorMQTT.loop();
}

void Publicador::SerialInfo(bool mode){
    this->Publicador_info = mode;
}