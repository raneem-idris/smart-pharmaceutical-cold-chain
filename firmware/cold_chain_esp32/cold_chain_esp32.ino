#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>

#define RFID_SS 5
#define RFID_RST 27

#define DHT_PIN 26
#define DHT_TYPE DHT11

MFRC522 rfid(RFID_SS, RFID_RST);
Adafruit_BMP085 bmp;
DHT dht(DHT_PIN, DHT_TYPE);

struct SensorData {
  String rfidUID;
  String cardType;
  float dhtTemperature;
  float humidity;
  float bmpTemperature;
  float pressure;
};

SensorData sensorData;

SemaphoreHandle_t dataMutex;

void setupRFID() {
  SPI.begin(18, 19, 23);
  rfid.PCD_Init();

  Serial.println("RFID Ready");
}

void setupBMP() {
  Wire.begin(21, 22);

  if (bmp.begin()) {
    Serial.println("BMP180 Ready");
  } else {
    Serial.println("BMP180 NOT FOUND!");
  }
}

void setupDHT() {
  dht.begin();

  Serial.println("DHT11 Ready");
}

String getRFIDUID() {
  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      uid += ":";
    }
  }

  uid.toUpperCase();

  return uid;
}

void readRFID() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = getRFIDUID();

  MFRC522::PICC_Type type;
  type = rfid.PICC_GetType(rfid.uid.sak);

  String cardType = rfid.PICC_GetTypeName(type);

  if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
    sensorData.rfidUID = uid;
    sensorData.cardType = cardType;

    xSemaphoreGive(dataMutex);
  }

  Serial.println();
  Serial.println("RFID");
  Serial.print("UID: ");
  Serial.println(uid);
  Serial.print("Card Type: ");
  Serial.println(cardType);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void readDHT() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT11 reading failed!");
    return;
  }

  if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
    sensorData.dhtTemperature = temperature;
    sensorData.humidity = humidity;

    xSemaphoreGive(dataMutex);
  }

  Serial.println();
  Serial.println("DHT11");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void readBMP() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();

  if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
    sensorData.bmpTemperature = temperature;
    sensorData.pressure = pressure;

    xSemaphoreGive(dataMutex);
  }

  Serial.println();
  Serial.println("BMP180");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" Pa");

  Serial.print("Pressure: ");
  Serial.print(pressure / 100.0);
  Serial.println(" hPa");
}

void readSensors() {
  readRFID();
  readDHT();
  readBMP();
}

void sensorTask(void *parameter) {
  while (true) {
    readSensors();

    Serial.println("----------------------------");

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void networkTask(void *parameter) {
  while (true) {
    SensorData data;

    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      data = sensorData;

      xSemaphoreGive(dataMutex);
    }

    Serial.println();
    Serial.println("NETWORK TASK");
    Serial.println("Core: 1");
    Serial.print("RFID: ");
    Serial.println(data.rfidUID);
    Serial.print("Temperature: ");
    Serial.println(data.dhtTemperature);
    Serial.print("Humidity: ");
    Serial.println(data.humidity);
    Serial.print("BMP Temperature: ");
    Serial.println(data.bmpTemperature);
    Serial.print("Pressure: ");
    Serial.println(data.pressure);

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  dataMutex = xSemaphoreCreateMutex();

  setupRFID();
  setupBMP();
  setupDHT();

  Serial.println();
  Serial.println("System Ready");

  xTaskCreatePinnedToCore(
    sensorTask,
    "Sensor Task",
    4096,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    networkTask,
    "Network Task",
    4096,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
