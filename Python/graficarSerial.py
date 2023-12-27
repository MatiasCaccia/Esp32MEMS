import serial
import matplotlib.pyplot as plt
from drawnow import drawnow

# Configuración del puerto serial
puerto_serial = serial.Serial('/dev/ttyACM0', 115200)  # Reemplaza 'COMx' con el nombre de tu puerto serial

# Configuración de la gráfica
plt.ion()  # Modo interactivo para actualizar en tiempo real
datos = []

# Función para graficar
def graficar():
    plt.title('Gráfica en Tiempo Real')
    plt.xlabel('Tiempo')
    plt.ylabel('Amplitud')
    plt.plot(datos, label='Datos en tiempo real')
    plt.legend(loc='upper left')

# Bucle principal
while True:
    while puerto_serial.inWaiting() == 0:
        pass  # Espera a recibir datos del puerto serial

    lectura = puerto_serial.readline().decode('utf-8').strip("!").rstrip()
    valor = float(lectura)

    datos.append(valor)
    if len(datos) > 50:
        datos.pop(0)  # Limita el número de datos mostrados en el gráfico

    drawnow(graficar)  # Actualiza la gráfica en tiempo real

