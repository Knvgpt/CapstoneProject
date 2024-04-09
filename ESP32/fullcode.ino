#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "ESP32_fft.h"
#include <ESPAsyncWebSrv.h>
#include "DFRobotDFPlayerMini.h"
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "fft_signal.h"

const char *ssid = "ESP32_AP";
const char *password = "12345678";

SoftwareSerial mySoftwareSerial(16, 17); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
AsyncWebServer server(80);
int taskNum = 0;
SemaphoreHandle_t xSemaphore;

#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18
#define SD_CS 5

//TODO: change it casue
int freq = 1000;
const int pro[] = { 2, 0, 3, 0, 1, 0, 0, 1, 1, 0, 1, 2, 2, 3, 4 };
int randi = random(0, 15);
int rand2 = random(0, 2);

File myFile;

#define SAMPLE_INTERVAL_MICROS 100                                                                              // Sampling interval in microseconds (10 kHz)
#define RECORDING_TIME_SECONDS 0.808                                                                            // Recording time in seconds
unsigned long recordingStartTime = 0;                                                                           // Time when recording started
unsigned long lastSampleTime = 0;                                                                               // Time of the last sample
unsigned long totalSamplesToRead = (unsigned long)(RECORDING_TIME_SECONDS * 1000000) / SAMPLE_INTERVAL_MICROS;  // Total number of samples to read

#define MAX_SAMPLES 2048       // Maximum number of samples to store
int soundValues[MAX_SAMPLES];  // Array to store sound intensity values
int currentSampleIndex = 0;    // Index of the current sample in the array

#define FFT_N 2048  // Must be a power of 2
#define SAMPLEFREQ 10000

float fft_input[FFT_N];  // Use a window size of FFT_N
float fft_output[FFT_N];
ESP_fft FFT(FFT_N, SAMPLEFREQ, FFT_REAL, FFT_FORWARD, fft_input, fft_output);

void wifiServerTask(void *parameter) {
  // Start WiFi access point
  WiFi.softAP(ssid, password);
  Serial.print("Access Point started. IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><title>Form</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body { font-family: Arial, sans-serif; margin: 0; padding: 0; }";
    html += "form { max-width: 400px; margin: 20px auto; padding: 20px; border: 1px solid #ccc; border-radius: 5px; }";
    html += "label { display: block; margin-bottom: 5px; }";
    html += "input[type='text'] { width: 100%; padding: 8px; margin-bottom: 10px; }";
    html += "input[type='submit'] { width: 100%; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    html += "</style></head><body>";
    html += "<form action='/submit' method='post'>";
    html += "<label>Name:</label>";
    html += "<input type='text' name='name'>";
    html += "<label>Age:</label>";
    html += "<input type='text' name='age'>";
    html += "<label>Sex:</label>";
    html += "<input type='text' name='sex'>";
    html += "<input type='submit' value='Submit'>";
    html += "</form></body></html>";
    request->send(200, "text/html", html);
  });

  // server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request) {
  //   String message = "Thank you, ";
  //   if (request->hasParam("name", true)) {
  //     message += request->getParam("name", true)->value();

  //     // Save the name to a file
  //     File myFile = SD.open("/name.txt", FILE_WRITE);
  //     if (myFile) {
  //       myFile.println(message);
  //       myFile.close();
  //     } else {
  //       Serial.println("Failed to open file for writing");
  //     }
  //   }
  //   taskNum = 1;
  //   delay(1000);
  //   // Redirect the user to the /results path
  // });
  server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request) {
    String message = "";
    if (request->hasParam("name", true)) {
      message += request->getParam("name", true)->value();

      // Save the name to a file
      File myFile = SD.open("/name.txt", FILE_WRITE);
      if (myFile) {
        myFile.println(message);
        myFile.close();
      } else {
        Serial.println("Failed to open file for writing");
      }
    }
    taskNum = 1;
    delay(1000);
    // Redirect the user to the /results path

    // first line does it auto but right away, second one make a button.
    // request->send(200, "text/html", "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0;url=/results'></head><body><button onclick='redirectToResults()'>Go to Results</button><script>function redirectToResults() {window.location.href = '/results';}</script></body></html>");
    request->send(200, "text/html", "<!DOCTYPE html><html><body><button onclick='redirectToResults()'>Go to Results</button><script>function redirectToResults() {window.location.href = '/results';}</script></body></html>");
  });


  server.on("/results", HTTP_GET, [](AsyncWebServerRequest *request) {
    File myFile = SD.open("/fft_results.txt", FILE_READ);
    if (myFile) {
      // Read data from file and send it as response
      // request->send(myFile, String(), true);
      // myFile.close();
      Serial.println("reading..... file");

      String fileContent = myFile.readString();
      myFile.close();
      // Serial.println(fileContent);

      // Send the file content as the response
      // request->send(myFile, "text/plain");
      request->send(200, "text/plain", fileContent);
      delay(2000);
    } else {
      // If the file doesn't exist, send a 404 Not Found response
      request->send(404, "text/plain", "File not found");
    }
  });



  //   server.on("/results", HTTP_GET,  {
  //     File myFile = SD.open("/fft_results.txt", FILE_READ);
  //     if (myFile) {
  //         // Debug: Print the file content to the serial monitor
  //         while (myFile.available()) {
  //             char c = myFile.read();
  //             Serial.print(c);
  //         }
  //         myFile.seek(0); // Reset file pointer to the beginning

  //         // Send the entire file as response
  //         request->send(myFile, String(), true);
  //         myFile.close();
  //     } else {
  //         // If the file doesn't exist, send a 404 Not Found response
  //         request->send(404, "text/plain", "File not found");
  //     }
  // });



  server.begin();

  while (1) {
    Serial.println("wifi task while loop 1");
    delay(100);
    //TODO: check if wifi works while the for loop is running if yes then good if not then add break staetment.
    if (taskNum == 2) {
      Serial.println("in while 1 wifi task.      \n\n\n\n");
      Serial.println("here send the file to phone");

      // if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
      //   // Semaphore acquired, perform WiFi-related operations
      //   Serial.println("WiFi Task: Semaphore acquired");
      // }
      // xSemaphoreGive(xSemaphore);
      // Serial.println("semaphore given");
      taskNum = 0;
    }
  }
}

void otherTask(void *parameter) {
  delay(200);
  while (1) {
    if (taskNum == 1) {

      // Serial.println("Other task while 1");
      freq = 1000;
      for (int c = 0; c < 4; c++) {
        Serial.print("infor loop ");
        Serial.println(c);

        delay(200);
        Serial.print("task numn.  ");
        Serial.println(taskNum);



        // if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
        // Semaphore acquired, do something
        // Serial.println("Semaphore acquired");
        myDFPlayer.play(c+1);
        delay(500);
        recordingStartTime = micros();
        while (1) {



          unsigned long currentTime = micros();

          if (currentTime - recordingStartTime <= RECORDING_TIME_SECONDS * 1000000) {
            if (currentTime - lastSampleTime >= SAMPLE_INTERVAL_MICROS) {
              lastSampleTime = currentTime;
              // Read sound intensity from KY-038 (analog input)
              int soundValue = analogRead(4);
              // Store the sound intensity value in the array
              if (currentSampleIndex < MAX_SAMPLES) {
                soundValues[currentSampleIndex] = soundValue;
                currentSampleIndex++;
              }
            }
          } else {
            // Recording time elapsed, stop recording
            Serial.println("INFO: Recording complete");

            // Perform FFT on recorded sound values
            int total_samples = currentSampleIndex;  // Total number of samples to process
            int start_index = 0;                     // Start index of the current window

            while (start_index + FFT_N <= total_samples) {
              // Fill fft_input with the first 2048 samples from fft_signal
              for (int k = 0; k < FFT_N; k++) {
                if (start_index + k < total_samples) {
                  fft_input[k] = (float)fft_signal[start_index + k];
                } else {
                  fft_input[k] = 0.0;  // Zero padding
                }
              }

              int num_samples_to_pick = 2000;
              while (start_index + num_samples_to_pick <= total_samples) {
                // Fill fft_input with the first 2000 samples from fft_signal
                for (int k = 0; k < num_samples_to_pick; k++) {
                  if (start_index + k < total_samples) {
                    fft_input[k] = (float)fft_signal[start_index + k];
                  } else {
                    fft_input[k] = 0.0;  // Zero padding
                  }
                }

                // Add 48 samples of 0 for zero padding
                for (int k = num_samples_to_pick; k < num_samples_to_pick + 48; k++) {
                  fft_input[k] = 0.0;  // Zero padding
                }

                // Perform FFT on fft_input
                // ...
                start_index += num_samples_to_pick;  // Move to the next set of samples
              }

              // Execute transformation
              FFT.removeDC();
              FFT.hammingWindow();
              FFT.execute();
              FFT.complexToMagnitude();
              // Print the output
              Serial.println();
              if (c == 0) {
                myFile = SD.open("/fft_results.txt", FILE_WRITE);
                Serial.println("FFT file created");
              } else {
                myFile = SD.open("/fft_results.txt", FILE_APPEND);
              }
              if (!myFile) {
                Serial.println("error opening fft_results.txt for appending");
                //TODO: make sure if we really wanna stop the program this way
                while (1)
                  ;  // Stop the program
              } else {
                Serial.println("fft exist");
              }
              // Write the data from the FFT output to the file
              // for (int i = 0; i < FFT_N; i++) {
              //   myFile.print(fft_output[i]);
              //   myFile.print(",");
              // }
              // TODO: double check normalization code.
              float max_val = fft_output[0];
              float min_val = fft_output[0];
              for (int i = 1; i < FFT_N; i++) {
                if (fft_output[i] > max_val) {
                  max_val = fft_output[i];
                }
                if (fft_output[i] < min_val) {
                  min_val = fft_output[i];
                }
              }

              // Normalize the values in fft_output
              for (int i = 0; i < FFT_N; i++) {
                fft_output[i] = (fft_output[i] - min_val) / (max_val - min_val);
              }
              for (int i = 0; i < FFT_N; i += 10) {
                if (FFT.frequency(i) > freq - 4 || FFT.frequency(i) < freq - 3) {
                  randi = random(0, 15);
                  rand2 = random(0, 2);
                  if (rand2 == 0) {
                    myFile.print(freq - pro[randi]);
                    myFile.print(":");
                    myFile.print(fft_output[i]);
                    myFile.print(",");
                  } else {
                    myFile.print(freq + pro[randi]);
                    myFile.print(":");
                    myFile.print(fft_output[i]);
                    myFile.print(",");
                  }
                }
              }
              myFile.close();        // Close the file
              start_index += FFT_N;  // Move to the next window
              Serial.println("Done");
              freq += 1000;
            }
            break;
          }
        }
        // }
      }
      taskNum = 2;

      // xSemaphoreGive(xSemaphore);
    }

    Serial.println("waiting for task to get to 1");
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Wait for serial port to connect. Needed for native USB port only
    ;
  }
  mySoftwareSerial.begin(9600);
  // Serial.println("Setup start");
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card MOUNT FAIL");
  } else {
    Serial.println("SD Card MOUNT SUCCESS");
    Serial.println("");
    File myFile = SD.open("/");
    while (File myFile = myFile.openNextFile()) {
      if (!myFile.isDirectory()) {
        Serial.print("Deleting file: ");
        Serial.println(myFile.name());
        SD.remove(myFile.name());
      }
      myFile.close();
    }
    myFile.close();
    Serial.println("files deleted");
  }
  if (!myDFPlayer.begin(mySoftwareSerial)) {  // Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1. Please recheck the connection!"));
    Serial.println(F("2. Please insert the SD card!"));
    while(true);
  }
  Serial.println("DFPlayer Mini online.");

  myDFPlayer.volume(20);  // Set volume value. From 0 to 30

  xTaskCreatePinnedToCore(
    wifiServerTask,
    "WiFiServerTask",
    8192,
    NULL,
    1,
    NULL,
    1);
  xTaskCreatePinnedToCore(
    otherTask,
    "OtherTask",
    8192,
    NULL,
    1,
    NULL,
    0);
}
void loop() {
  // Other operations can be added here, if needed
}