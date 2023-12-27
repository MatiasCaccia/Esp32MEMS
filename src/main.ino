/*
 * Sketch samples audio data from I2S microphone, processes the data 
 * with digital IIR filters and calculates A or C weighted Equivalent 
 * Continuous Sound Level (Leq)
 * 
 * I2S is setup to sample data at Fs=48000KHz (fixed value due to 
 * design of digital IIR filters). Data is read from I2S queue 
 * in 'sample blocks' (default 125ms block, equal to 6000 samples) 
 * by 'i2s_reader_task', filtered trough two IIR filters (equalizer 
 * and weighting), summed up and pushed into 'samples_queue' as 
 * sum of squares of filtered samples. 
 * The main task then pulls data from the queue and calculates decibel 
 * value relative to microphone reference amplitude, derived from 
 * datasheet sensitivity dBFS value, number of bits in I2S data, and 
 * the reference value for which the sensitivity is specified (typically 
 * 94dB, pure sine wave at 1KHz).
 * 
 * Displays line on the small OLED screen with 'short' LAeq(125ms)
 * response and numeric LAeq(1sec) dB value from the signal RMS.
 */

#include <driver/i2s.h>
#include "sos-iir-filter.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "wifi_manager.h"
#include <XSpace.h>


//
// Configuración
//

#define LEQ_PERIOD        1           // segundos de integración(s)
#define WEIGHTING         A_weighting // Ponderación frecuencial. Está también: 'C_weighting' y 'None' (Z_weighting)
#define LEQ_UNITS         "LAeq"      // Variable en función del la ponderación
#define DB_UNITS          "dBA"       // Unidad en función de la ponderación
#define USE_DISPLAY       0           // Se muestra la información en un display (1) o no (0)

//
#define MIC_EQUALIZER     INMP441    // Filtro IIR de compensación aplicado. 'None' deshabilita el filtro 
#define MIC_OFFSET_DB     3.0103     // Offset (sine-wave RMS vs. dBFS) - Factor de calibración lineal

// Información del Datasheet
#define MIC_SENSITIVITY   -26         // Valor dBFS esperado en la referencia (Sensibilidad)
#define MIC_REF_DB        94.0        // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_OVERLOAD_DB   116.0       // Valor máximo registrable en dB
#define MIC_NOISE_DB      29          // Piso de ruido
#define MIC_BITS          24          // Cantidad de bits en la transferencia del I2S
//Macro para desplazamiento de datos. Se acomoda el dato ingresado en el largo especifico del micrófono
#define MIC_CONVERT(s)    (s >> (SAMPLE_BITS - MIC_BITS))
#define MIC_TIMING_SHIFT  0           // Vale 0 todo el tiempo o 1 para corregir el tiempo la lectura del
                                      // bit más significativo de ciertos micrófonos

// Calculo de la amplitud de referencia al compilar
// Algo así como 333873.13 
constexpr double MIC_REF_AMPL = pow(10, double(MIC_SENSITIVITY)/20) * ((1<<(MIC_BITS-1))-1);

//
// I2S pins
// Se puede conectar a casi cualquier pin
// SD a cualquier pin incluidas las entradas en pins 36 a 39
// SCK, BCLK, WS, L/R y CLK deben ir a pines de salida
//
#define I2S_WS            18 
#define I2S_SCK           23 
#define I2S_SD            19 
// L/R a GND

// Periférico I2S (0 o 1)
#define I2S_PORT          I2S_NUM_0

//
// Configuración Wifi + MQTT
// 
//WiFiManager wifiManager;
//MQTTManager mqttManager("192.168.0.61", 1883);
char buffer[50];
double mqttvel;
XSpaceV2 XSpaceV2Board;
const char* topic_2 = "Callback";


// ----------------------
// Filtro de Compensación
// ----------------------

// Se emplean filtro SOS
// Se asume que b0 y a0 son 1.0
// Se aplica una ganancia unitaria al final
// B and A coefficients were transformed with GNU Octave: 
// [sos, gain] = tf2sos(B, A)
// See: https://www.dsprelated.com/freebooks/filters/Series_Second_Order_Sections.html
// NOTE: SOS matrix 'a1' and 'a2' coefficients are negatives of tf2sos output
//

// InvenSense INMP441
// Datasheet: https://www.invensense.com/wp-content/uploads/2015/02/INMP441.pdf
// B ~= [1.00198, -1.99085, 0.98892]
// A ~= [1.0, -1.99518, 0.99518]
SOS_IIR_Filter INMP441 = {
  gain: 1.00197834654696, 
  sos: { // Second-Order Sections {b1, b2, -a1, -a2}
    {-1.986920458344451, +0.986963226946616, +1.995178510504166, -0.995184322194091}
  }
};


// ---------------------
// FILTRO DE PONDERACION
// ---------------------
// Ponderación A con fs = 48000
// (By Dr. Matt L., Source: https://dsp.stackexchange.com/a/36122)
// B = [0.169994948147430, 0.280415310498794, -1.120574766348363, 0.131562559965936, 0.974153561246036, -0.282740857326553, -0.152810756202003]
// A = [1.0, -2.12979364760736134, 0.42996125885751674, 1.62132698199721426, -0.96669962900852902, 0.00121015844426781, 0.04400300696788968]

/**
 * b = array([ 0.14250933,  0.04299392, -1.59706895,  3.10815007, -2.30961559, 0.61303123]), 
 * a = array([ 1.  , -4.293876,  7.28471708, -6.12627174,  2.6104718, -0.51161179,  0.03657066]))
 * */

SOS_IIR_Filter A_weighting = {
  gain: 0.169994948147430, 
  sos: { // Second-Order Sections {b1, b2, -a1, -a2}
    {-2.00026996133106, +1.00027056142719, -1.060868438509278, -0.163987445885926},
    {+4.35912384203144, +3.09120265783884, +1.208419926363593, -0.273166998428332},
    {-0.70930303489759, -0.29071868393580, +1.982242159753048, -0.982298594928989}
  }
};

// Ponderación C con fs = 48000
// B = [-0.49164716933714026, 0.14844753846498662, 0.74117815661529129, -0.03281878334039314, -0.29709276192593875, -0.06442545322197900, -0.00364152725482682]
// A = [1.0, -1.0325358998928318, -0.9524000181023488, 0.8936404694728326   0.2256286147169398  -0.1499917107550188, 0.0156718181681081]

SOS_IIR_Filter C_weighting = {
  gain: -0.491647169337140,
  sos: { 
    {+1.4604385758204708, +0.5275070373815286, +1.9946144559930252, -0.9946217070140883},
    {+0.2376222404939509, +0.0140411206016894, -1.3396585608422749, -0.4421457807694559},
    {-2.0000000000000000, +1.0000000000000000, +0.3775800047420818, -0.0356365756680430}
  }
};


// ---------------------------------
// PROPIEDADES DE LAS MUESTRAS Y DMA
// ---------------------------------
#define SAMPLE_RATE       48000 
#define SAMPLE_BITS       32
#define SAMPLE_T          int32_t // Tipo de dato a utilizar para almacenar cada muestra de audio
#define SAMPLES_SHORT     (SAMPLE_RATE / 8) // 125ms de muestras -> 6000 muestras
#define SAMPLES_LEQ       (SAMPLE_RATE * LEQ_PERIOD)// Muestras a integrar para obtener el Leq
#define DMA_BANK_SIZE     (SAMPLES_SHORT / 16) // Tamaño del Direct Memory Acces Controller: 375 muestras (1.5 KByte)
#define DMA_BANKS         32 // Cantidad de bancos utilizados

// ---------------------------------
// ESTRUCTURA DE ALMACENAMIENTO
// ---------------------------------
// Datos que se colocan en la cola
struct sum_queue_t {
  // Estructura para almacenar datos de mediciones ecualizadas y ponderadas
  float sum_sqr_SPL; // Suma de los cuadrados de las muestras ecualizadas
  float sum_sqr_weighted; // Suma de los cuadrados de las muestras ponderadas
  uint32_t proc_ticks; // Número de Tics empleados en procesar la información
};
QueueHandle_t samples_queue; // Cola para mover los elementos de la estructura anterior

// Buffer de memoria estático que almacena muestras
// Array con 6k floats alineado en límites de memoria de 4 bytes (mejora el rendimiento)
float samples[SAMPLES_SHORT] __attribute__((aligned(4))); 

// --------------------------------
// CONFIGURACION DE ENTRADA POR I2S
// --------------------------------
void mic_i2s_init() {
  const i2s_config_t i2s_config = {
    // Estructura de configuración de I2S
    mode: i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), //ESP32 es Maestro y receptor de datos
    sample_rate: SAMPLE_RATE,
    bits_per_sample: i2s_bits_per_sample_t(SAMPLE_BITS),
    channel_format: I2S_CHANNEL_FMT_ONLY_LEFT, //Solo se usa el canal LEFT
    communication_format: i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    intr_alloc_flags: ESP_INTR_FLAG_LEVEL1,
    dma_buf_count: DMA_BANKS,
    dma_buf_len: DMA_BANK_SIZE,
    use_apll: true,
    tx_desc_auto_clear: false,
    fixed_mclk: 0
  };
  // Pines I2S
  const i2s_pin_config_t pin_config = {
    bck_io_num:   I2S_SCK,  
    ws_io_num:    I2S_WS,    
    data_out_num: -1, // not used
    data_in_num:  I2S_SD   
  };
  //Instalación del controlador I2S con l puerto especificado y la configuración anterior
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  // Configuración de los pines de comunicación por I2S
  i2s_set_pin(I2S_PORT, &pin_config);
}

// ----------------------------------
// TAREA DE LECTURA POR PROTOCOLO I2S
// ----------------------------------
// Se crea una task para la lectura de las muestras por I2S para que el 
// procesamiento del filtro IIR pueda programarse en un núcleo distinto.
// Así se logran realizar otras tareas mientras que se recopila audio.
// Esta metodología logra aumentar la eficiencia del código
//
// Esta tarea es de alta prioridad, por lo que se configura para que realice 
// mínimo trabjo hasta comprimir la información en una suma de cuadrados
//
#define I2S_TASK_PRI   4 // Prioridad de la tarea
#define I2S_TASK_STACK 2048 //Tamaño de pila de la tarea (2048 palabras de 32 bits)
//
void mic_i2s_reader_task(void* parameter) {
  // Función que se ejecuta en paralelo con la tarea principal
  // El argumento de entrada no se utiliza
  
  // Inicialización de la captura de muestras con la configuración previa
  mic_i2s_init(); 
  
  //Se descarta el primer bloque de datos de inicio (INMP441 tiene hasta 83ms)
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);
  Serial.printf("Eliminar primer buffer");
  
  while (true) {
    // Bucle de ejecución continua que toma valores del Mic por I2S
    //
    // Se mueven las muestras del buffer DMA a los buffers de muestras
    // Cuando se llega a un tamaño determinado, se desbloquea la tarea
    // Esto habilita el procesamiento de las muestras recibidas
    i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(SAMPLE_T), &bytes_read, portMAX_DELAY);

    // Marcar el tiempo de inicio para el procesamiento de las muestras
    TickType_t start_tick = xTaskGetTickCount();
    
    // int_samples apunta al mismo bloque de memoria que samples pero se lo interpreta como un conjunto de valores int32_t
    SAMPLE_T* int_samples = (SAMPLE_T*)&samples;
    // Desplazamiento a la derecha de los 32 bits de cada muestra samples[i] para ajustar la precisión a 24 bits
    for(int i=0; i<SAMPLES_SHORT; i++) samples[i] = MIC_CONVERT(int_samples[i]);
    
    //for (int i = 0; i < SAMPLES_SHORT; i++) Serial.println(samples[i]);
    
    sum_queue_t q;
    // Aplicar ecualización y calcular la suma ponderada de cuadrados, 
    // escribe las muestras filtradas de vuelta en el mismo búfer.
    q.sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

    // Aplicar ponderación y calcular la suma ponderada de cuadrados.
    q.sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);

    // Solo para depuración. Ticks que pasamos filtrando y sumando bloque de datos I2S.
    q.proc_ticks = xTaskGetTickCount() - start_tick;

    // Enviar las sumas a la cola FreeRTOS donde la tarea principal las recogerá
    // y calculará posteriormente los valores en decibelios (división, logaritmos, etc.).
    xQueueSend(samples_queue, &q, portMAX_DELAY);
    
  }
}

void Mqtt_Callback(char* topic, byte* payload, unsigned int length) { 
  
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.println(topic);
  
  for(int i=0; i<length; i++){
    incoming = incoming + (char) payload[i];
  }
  Serial.println("Mensaje: " + incoming);
}

void MqttTask(void *pvParameters){
  while(1){
    if (!XSpaceV2Board.Mqtt_IsConnected()){
      Serial.println("Intentando conectar MQTT");
      XSpaceV2Board.Mqtt_Connect("ESP32Test","Mati","1234"); 
      XSpaceV2Board.Mqtt_Suscribe("Callback");
    }
    if (XSpaceV2Board.Mqtt_IsConnected()){
      //mqttvel= 115; //XSpaceV2Board.GetEncoderSpeed (DEGREES_PER_SECOND);
      //((String)mqttvel).toCharArray(buffer, 10);
      //XSpaceV2Board.Mqtt_Publish("test", buffer);
      Serial.print("!");
      vTaskDelay(1000);
    }
  XSpaceV2Board.Mqtt_KeepAlive();
  }
  vTaskDelete(NULL);

}



// ----------------------------------
//               SETUP
// ----------------------------------
void setup() {
  setCpuFrequencyMhz(80); // Configurar frecuencia del procesador [MHz]
  
  Serial.begin(112500);
  delay(1000); // Tiempo de espera por seguridad
  
  // Configuración WiFi
  //wifiManager.setSSID("Fibertel WiFi748 2.4GHz");
  //wifiManager.setPassword("01439656713");

  // Crear cola de FreeRTOS
  samples_queue = xQueueCreate(8, sizeof(sum_queue_t));
  
  // Crear las task de FreeRTOS
  //xTaskCreatePinnedToCore(WiFiManager::taskFunction, "WiFiTask", 2048, &wifiManager, 1, NULL,0);
  XSpaceV2Board.Wifi_init("Fibertel WiFi748 2.4GHz","01439656713");
  XSpaceV2Board.Mqtt_init("192.168.0.231",1883);

  xTaskCreatePinnedToCore(MqttTask, "MQTTTask", 4000, NULL, 1, NULL,1);
  
  /*
  // Configuración MQTT
  WifiClient espClient;
  PubSubClient client(espClient);
  const char* mqtt_server = "192.168.0.61";
  const int mqtt_port = 1883;
  const char* mqtt_topic = "Sensor";
  client.setServer(mqtt_server, mqtt_port);
  client.connect("Esp32");
  */

  xTaskCreate(mic_i2s_reader_task, "Mic I2S Reader", I2S_TASK_STACK, NULL, I2S_TASK_PRI, NULL);
  
  sum_queue_t q;
  uint32_t Leq_samples = 0;
  double Leq_sum_sqr = 0;
  double Leq_dB = 0;

  // Lectura de suma de muestras de 'i2s_reader_task'
  while (xQueueReceive(samples_queue, &q, portMAX_DELAY)) {

    // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
    double short_RMS = sqrt(double(q.sum_sqr_SPL) / SAMPLES_SHORT);
    // Revisión obligatoria a esta fórmula:
    double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);

    // "Acoustic overflow" ?
    if (short_SPL_dB > MIC_OVERLOAD_DB) {
      Leq_sum_sqr = INFINITY;
    } else if (isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB)) {
      Leq_sum_sqr = -INFINITY;
    }

    // Acumulación de los valores Leq
    Leq_sum_sqr += q.sum_sqr_weighted;
    Leq_samples += SAMPLES_SHORT;

    // When we gather enough samples, calculate new Leq value
    if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD) {
      double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
      Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);
      Leq_sum_sqr = 0;
      Leq_samples = 0;
      
      // Serial output, customize (or remove) as needed
      Serial.printf("%.1f\n", Leq_dB);
      ((String)Leq_dB).toCharArray(buffer, 10);
      XSpaceV2Board.Mqtt_Publish("test", buffer);
      //------------MQTT------------
      //char message[16]; // Ajusta el tamaño según tus necesidades
      //snprintf(message, sizeof(message), "%.1f", Leq_dB);
      //mqttManager.publish("leq",message);
      
      // Debug only
      //Serial.printf("%u processing ticks\n", q.proc_ticks);
      /*
      if (client.connected()) {
        client.publish("Sensor", "Hola, ¿estás?");
      }else{
        Serial.print("No MQTT");
      }
      */
    }
  }
}

// ----------------------------------
//               SETUP
// ----------------------------------
void loop() {
  // Nothing here..
}