# Documentación
Este proyecto consiste en el uso de un micrófono INMP441 para obtener niveles de presión sonora. El código está dividido en diversos
archivos que se encargan de distintas funcionalidades. A continuación, se proporciona una descripción detallada de cada archivo y su funcionalidad.

## Archivo main.ino
Este código implementa un sistema de medición de niveles de sonido en decibelios utilizando un micrófono I2S (INMP441) y filtros digitales IIR (Infinito Impulso de Respuesta). Además, se envían los resultados a través de MQTT y se realiza una conexión WiFi. 

### Tareas realizadas:
1. Se incluyen las bibliotecas necesarias y se definen configuraciones y constantes.
2. Se definen los filtros IIR para el micrófono INMP441, y los filtros de ponderación A y C.
3. Se establece la configuración para la conexión I2S y se crea una tarea (`mic_i2s_reader_task`) para leer continuamente las muestras del micrófono.
4. Hay una tarea (`MqttTask`) que gestiona la conexión MQTT.
5. En el bucle principal, se procesan las muestras recibidas, se calculan los niveles de sonido ponderados y se envían los resultados a través de MQTT.
6. La conexión WiFi se inicia y se configura el MQTT.

### Consideraciones generales:
- El código utiliza FreeRTOS para gestionar tareas y colas.
- La conexión WiFi y MQTT se gestionan a través de la clase `Publicador`.
- Se utilizan filtros IIR para ecualizar el micrófono y aplicar ponderación A y C.
- Los resultados se envían a través de MQTT con un tema llamado "test".
- Los resultados se imprimen en la consola serial.

## Archivo Publicador
### Archivo Publicador.h

El archivo Publicador.h contiene la declaración de la clase Publicador junto con sus métodos y atributos. Aquí se proporciona una breve descripción de la clase y sus métodos:

- Publicador class: Gestiona la conexión WiFi y la comunicación MQTT en un dispositivo Arduino. 

Métodos públicos:

- SerialInfo(bool mode): Habilita o deshabilita la impresión de información (modo verbose).
- Wifi_init(const char* ssid, const char* password): Inicializa la conexión WiFi con los parámetros proporcionados.
- Mqtt_init(const char* mqtt_server, uint16_t mqtt_port): Configura la conexión al servidor MQTT con la dirección y el puerto especificados.
- Mqtt_Connect(const char *clientId, const char *mqtt_user, const char *mqtt_pass): Intenta conectar al servidor MQTT con los credenciales proporcionados.
- Mqtt_Publish(const char* topic, const char* payload): Publica un mensaje en el servidor MQTT con el tema y el contenido proporcionados.
- Mqtt_Suscribe(const char* topic): Se suscribe a un tema en el servidor MQTT si está conectado.
- Mqtt_KeepAlive(): Mantiene activa la conexión MQTT y procesa los mensajes entrantes.
- Mqtt_IsConnected() -> bool: Verifica si está conectado al servidor MQTT y devuelve true si la conexión está activa, false en caso contrario.

### Archivo Publicador.cpp
El archivo Publicador.cpp contiene la implementación de la clase Publicador, que gestiona la conexión WiFi y la comunicación MQTT en un dispositivo Arduino. 
Aquí se proporciona una descripción de cada función implementada:

- Wifi_init(const char* ssid, const char* password): Inicializa la conexión WiFi con los parámetros proporcionados, como el SSID y la contraseña de la red WiFi.
- Mqtt_Connect(const char *clientId, const char *mqtt_user, const char *mqtt_pass): Intenta conectar al servidor MQTT con los credenciales proporcionados, como el identificador único del cliente, nombre de usuario y contraseña.
- Mqtt_init(const char *mqtt_server, uint16_t mqtt_port): Configura la conexión al servidor MQTT con la dirección IP o el nombre de dominio del servidor y el puerto especificado.
- Mqtt_IsConnected() -> bool: Verifica si está conectado al servidor MQTT y devuelve true si la conexión está activa, false en caso contrario.
- Mqtt_Publish(const char* topic, const char* payload): Publica un mensaje en el servidor MQTT con el tema y el contenido proporcionados.
- Mqtt_Suscribe(const char* topic): Se suscribe a un tema en el servidor MQTT si está conectado.
- Mqtt_KeepAlive(): Mantiene activa la conexión MQTT y procesa los mensajes entrantes.
- SerialInfo(bool mode): Habilita o deshabilita la impresión de información (modo verbose) según el valor booleano proporcionado.


## sos-iir-filter.h
El archivo sos-iir-filter.h contiene la implementación de un filtro IIR (Infinite Impulse Response) de segundo orden (SOS) para procesamiento de señales en un dispositivo Arduino. Este archivo proporciona una implementación flexible y optimizada de un filtro IIR SOS en el entorno del ESP32, ofreciendo funciones para filtrado, cálculo de suma de cuadrados y clases para gestionar filtros con múltiples secciones SOS y factores de ganancia. A continuación, se proporciona una descripción de las estructuras, funciones y clases presentes en el archivo:

### Estructuras
- SOS_Coefficients: Estructura que representa los coeficientes de un filtro SOS, con los valores b1, b2, a1 y a2.
- SOS_Delay_State: Estructura que mantiene el estado de retardo para un filtro SOS, con los valores w0 y w1.

### Funciones en lenguaje ensamblador (ASM)
- sos_filter_f32: Implementa un filtro SOS de segundo orden en el ESP32. Aplica el filtro a un conjunto de datos de entrada y devuelve el resultado en un conjunto de datos de salida.
- sos_filter_sum_sqr_f32: Similar a sos_filter_f32, pero también devuelve la suma de los cuadrados de los valores filtrados.

### Clase SOS_IIR_Filter
Métodos públicos:

#### Constructor
- SOS_IIR_Filter(size_t num_sos, const float gain, const SOS_Coefficients _sos[]): Constructor dinámico que inicializa un filtro IIR SOS con el número de secciones SOS, el factor de ganancia y los coeficientes proporcionados.
- template <size_t Array_Size> SOS_IIR_Filter(const float gain, const SOS_Coefficients (&sos)[Array_Size]): Constructor de plantilla para la declaración constante de un filtro.
- float filter(float* input, float* output, size_t len): Aplica el filtro IIR SOS a una serie de datos de entrada y escribe los valores filtrados en un conjunto de datos de salida. Devuelve la suma de los cuadrados de todos los valores filtrados.

#### Destructor
- ~SOS_IIR_Filter(): Libera la memoria asignada dinámicamente para los coeficientes y el estado de retardo.

### Clase No_IIR_Filter
Métodos públicos:

#### Constructor
- No_IIR_Filter(): Constructor que representa la ausencia de un filtro IIR.
- float filter(float* input, float* output, size_t len): Copia los datos de entrada a los datos de salida y devuelve la suma de los cuadrados de los valores de entrada.

### Objeto None
- No_IIR_Filter: Objeto predefinido que representa la ausencia de un filtro IIR. Puede utilizarse para comparaciones o como un filtro nulo.


# Hardware
## Conexión módulo INMP441 

 En orden:

 | VCC | GND | SCL | SDA | XSA | XCL | AD0 | INT | 
 |:---:|:---:|:--:|:-----:|:--:|:----:|:---:|:---:|
 |3.3 V| GND | I2C SCL | I2S SDA |NoConn| NoConn| NoConn |NoConn|


# Instrucciones de Uso
### Configuración Inicial:
Conectar el micrófono al ESP32 según lo indicado. 

### Calibración:
Registrar el nivel de un calibrador certificado y aplicar la corrección de forma manual