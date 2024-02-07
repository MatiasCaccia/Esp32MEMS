#include "SD_funciones.h"

SPIClass SPI2(HSPI);

/**
* @brief Función para el listado de los directorios existentes de forma recursiva
*
* @param fs Instancia del sistema de archivos a utilizar
* @param dirname Nombre del directorio a Listar
* @param levels Niveles de profunidad para listar de manera recursiva
*/
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) 
{
  Serial.printf("Listando el directorio: %s\n", dirname);

  File root = fs.open(dirname);
  
  if (!root) 
  {
    Serial.println("Error al abrir el directorio");
    return;
  }
  
  if (!root.isDirectory()) 
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  
  while (file) 
  {
    if (file.isDirectory()) 
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      
      if (levels) 
      {
        listDir(fs, file.path(), levels - 1);
      }
    } 
    else 
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    
    file = root.openNextFile();
  }
}

/**
* @brief Función para crear un nuevo directorio.
* 
* @param fs Instancia del sistema de archivos a utilizar
* @param path Ruta del nuevo directorio a crear
*/
void createDir(fs::FS &fs, const char * path) 
{
  Serial.printf("Creating Dir: %s\n", path);
  
  if (fs.mkdir(path)) 
  {
    Serial.println("Dir created");
  } 
  else 
  {
    Serial.println("mkdir failed");
  }
}


/**
 * @brief Función para eliminar un directorio
 * 
 * @param fs Instancia del sistema de archivos a utilizar
 * @param path Ruta del directorio a eliminar
 */

void removeDir(fs::FS &fs, const char * path) 
{
  Serial.printf("Removing Dir: %s\n", path);
  
  if (fs.rmdir(path)) 
  {
    Serial.println("Dir removed");
  } 
  else 
  {
    Serial.println("rmdir failed");
  }
}

/**
 * @brief Función para leer el contenido de un archivo
 *
 * @param fs Instancia del sistema de archivos a utilizar
 * @param path Ruta del archivo a leer
 *
 */
void readFile(fs::FS &fs, const char * path) 
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  
  if (!file) 
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  
  while (file.available()) 
  {
    Serial.write(file.read());
  }
  
  file.close();
}

/**
 * @brief Función para escribir en un archivo
 * 
 * @param fs Instancia del sistema de archivos a utilizar
 * @param path Ruta del archivo a escribir
 * @param message Mensaje a escribir en el archivo
 */
void writeFile(fs::FS &fs, const char * path, const char * message) 
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  
  if (!file) 
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  
  if (file.print(message)) 
  {
    Serial.println("File written");
  } 
  else 
  {
    Serial.println("Write failed");
  }
  
  file.close();
}

/**
 * @brief Función para agregar contenido a un archivo
 *
 * @param fs Instancia del sistema de archivos a utilizar
 * @param path Ruta del archivo a actualizar
 * @param message Mensaje a agregar al archivo
 */
void appendFile(fs::FS &fs, const char * path, const char * message) 
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  
  if (!file) 
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  
  if (file.print(message)) 
  {
    Serial.println("Message appended");
  } 
  else 
  {
    Serial.println("Append failed");
  }
  
  file.close();
}
/**
 * @brief Función para renombrar un archivo.
 * @param fs Instancia del sistema de archivos a utilizar.
 * @param path1 Ruta del archivo a renombrar.
 * @param path2 Nueva ruta con el nombre del archivo.
 */
void renameFile(fs::FS &fs, const char * path1, const char * path2) 
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  
  if (fs.rename(path1, path2)) 
  {
    Serial.println("File renamed");
  } 
  else 
  {
    Serial.println("Rename failed");
  }
}

/**
 * @brief Función para eliminar un archivo.
 * 
 * @param fs Instancia del sistema de archivos a utilizar.
 * @param path Ruta del archivo a eliminar.
 */
void deleteFile(fs::FS &fs, const char * path) 
{
  Serial.printf("Deleting file: %s\n", path);
  
  if (fs.remove(path)) 
  {
    Serial.println("File deleted");
  } 
  else 
  {
    Serial.println("Delete failed");
  }
}

/**
 * @brief Función para probar la entrada/salida de un archivo.
 *
 * @param fs Instancia del sistema de archivos a utilizar.
 * @param path Ruta del archivo a utilizar para la prueba.
 */
void testFileIO(fs::FS &fs, const char * path) 
{
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  
  if (file) 
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    
    while (len) 
    {
      size_t toRead = len;
      
      if (toRead > 512) 
      {
        toRead = 512;
      }
      
      file.read(buf, toRead);
      len -= toRead;
    }
    
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } 
  else 
  {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  
  if (!file) 
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  
  for (i = 0; i < 2048; i++) 
  {
    file.write(buf, 512);
  }
  
  end = millis() - start;
  
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

