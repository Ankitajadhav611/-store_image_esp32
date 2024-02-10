#include <Arduino.h>
#include <SPI.h>
#include "FS.h"
#include "SPIFFS.h"

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}
// Initialize variables
bool image_start_found = false;
uint8_t image_counter = 0;
uint8_t* image_data = nullptr;
size_t image_data_size = 0;


void setup() {
  Serial.begin(115200);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    while (1) {}  // Stop program execution if SPIFFS initialization fails
  }
  
  listDir(SPIFFS, "/", 0);
}

uint8_t temp = 0, temp_last = 0;
byte buf[256];
static int i = 0;
bool is_header = false;
uint8_t* jpeg_buffer;

void writeFile(const char* path, uint8_t* data, size_t length) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }

  if (file.write(data, length) != length) {
    Serial.println("Error writing to file");
  } else {
    Serial.println("File written successfully");
  }

  file.close();
}

void printFileContents(const char* path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Error opening file for reading");
    return;
  }

  Serial.println("File contents:");
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println("End of file contents");

  file.close();
}



void saveImage(uint8_t* data, size_t size) {
  // Save the received image data to a local file
  char filename[30];
  snprintf(filename, sizeof(filename), "/image_%d.jpg", image_counter++);
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }

  if (file.write(data, size) != size) {
    Serial.println("Error writing to file");
  } else {
    Serial.print("Image saved as ");
    Serial.println(filename);
  }

  file.close();
}

void loop() {
  // Read from serial port
  while (Serial.available()) {
    uint8_t byteRead = Serial.read();
    
    if (image_start_found) {
      // Append data to image_data buffer
      uint8_t* new_image_data = (uint8_t*)realloc(image_data, image_data_size + 1);
      if (new_image_data) {
        image_data = new_image_data;
        image_data[image_data_size++] = byteRead;
      } else {
        Serial.println("Memory allocation failed!");
        return;
      }

      // Search for the end of image marker (0xFFD9)
      if (byteRead == 0xD9 && image_data_size >= 2 && image_data[image_data_size - 2] == 0xFF) {
        // End of image received, save the image
        saveImage(image_data, image_data_size);
        
        // Reset variables for next image
        image_start_found = false;
        image_data_size = 0;
        free(image_data);
        image_data = nullptr;
      }
    } else {
      // Search for the start of image marker (0xFFD8)
      if (byteRead == 0xD8 && Serial.peek() == 0xFF) {
        // Start of image found
        image_start_found = true;
        // Allocate memory for image_data buffer
        image_data = (uint8_t*)malloc(2);
        if (!image_data) {
          Serial.println("Memory allocation failed!");
          return;
        }
        image_data[0] = 0xFF;
        image_data[1] = 0xD8;
        image_data_size = 2;
      }
    }
  }
}
