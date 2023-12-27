# Documentación
Este proyecto consiste en el uso de un micrófono INMP441 para obtener niveles de presión sonora. El código está dividido en diversos
archivos que se encargan de distintas funcionalidades. A continuación, se proporciona una descripción detallada de cada archivo y su funcionalidad.

## Archivo main.ino
Este archivo principal main.ino inicia el controlador MPUController que maneja la inicialización y procesamiento de datos del MPU6050 con DMP. Si la inicialización falla, el programa se detiene.

## Archivo Publicador.h - Publicador.cpp
El archivo MpuMati.h contiene la implementación de la clase MPUController, que controla la inicialización y procesamiento de datos del MPU6050 con DMP. También incluye métodos privados para configurar offsets y mostrar ángulos.

## sos-iir-filter.h
El archivo Calibrador.cpp contiene el programa de calibración para el sensor MPU6050. Permite ajustar los offsets del sensor para compensar errores y mejorar la precisión.

## Instrucciones de Uso

### Configuración Inicial:
En el archivo Calibrador.cpp, ajusta las variables buffersize, acel_deadzone, y giro_deadzone según tus necesidades.
Ejecuta el programa Calibrador.cpp en tu entorno de desarrollo.

### Calibración:
Coloca el MPU6050 en posición horizontal con las letras del paquete hacia arriba.
No toques el sensor hasta que veas el mensaje de finalización.
Verifica que las lecturas del sensor con offsets estén cerca de 0, 0, 16384, 0, 0, 0.
Anota los offsets calculados para su uso en otros proyectos.

### Integración en Proyectos:
Utiliza la clase MPUController en tu código principal (main.cpp) para acceder a los datos del MPU6050 con DMP.
Asegúrate de haber ajustado los offsets obtenidos durante la calibración para obtener mediciones precisas. 

## Conexión módulo MPU6050

 En orden:

 | VCC | GND | SCL | SDA | XSA | XCL | AD0 | INT | 
 |:---:|:---:|:--:|:-----:|:--:|:----:|:---:|:---:|
 |3.3 V| GND | I2C SCL | I2S SDA |NoConn| NoConn| NoConn |NoConn|



                                                                                                                                                                                                                                                                                                                                                                                                                