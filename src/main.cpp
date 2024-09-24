
#include <Adafruit_NeoPixel.h> // Para controlar tiras de LED NeoPixel
#include <Arduino.h>
#include <BleGamepad.h> // https://github.com/lemmingDev/ESP32-BLE-Gamepad

BleGamepad bleGamepad("DualProGamer1", "BratosDev", 100);

#define numOfButtons 11
#define numOfHats 1 // Maximum 4 hat switches supported

byte previousButtonStates[numOfButtons];
byte currentButtonStates[numOfButtons];
byte buttonPins[numOfButtons] = {22, 23, 17, 16, 4, 15, 12, 13, 14, 27};
byte physicalButtons[numOfButtons] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

byte previousHatStates[numOfHats * 4];
byte currentHatStates[numOfHats * 4];
byte hatPins[numOfHats * 4] = {5, 19, 18, 21}; // In order UP, LEFT, DOWN, RIGHT. 4 pins per hat switch (Eg. List 12 pins if there are 3 hat switches)
const int BATTERY_PIN = 35;                    // Pin para medir la batería
#define HID_GAMEPAD 0x03C4
// Pines para los LEDs
const int RED_PIN = 32;
const int GREEN_PIN = 26;
const int BLUE_PIN = 33;

// Configuración de PWM
const int PWM_FREQ = 10000;  // Frecuencia PWM
const int PWM_RES = 8;       // Resolución de 8 bits (valores de 0 a 255)
const int GREEN_CHANNEL = 0; // Canal para el LED verde
const int RED_CHANNEL = 1;   // Canal para el LED rojo
const int BLUE_CHANNEL = 2;  // Canal para el LED azul

void testRed()
{
  ledcWrite(RED_CHANNEL, 255); // Encender rojo
  delay(1000);
  ledcWrite(RED_CHANNEL, 0); // Apagar rojo
  delay(1000);
}

void testGreen()
{
  ledcWrite(GREEN_CHANNEL, 255); // Encender rojo
  delay(1000);
  ledcWrite(GREEN_CHANNEL, 0); // Apagar rojo
  delay(1000);
}

void testBlue()
{
  ledcWrite(BLUE_CHANNEL, 255); // Encender rojo
  delay(1000);
  ledcWrite(BLUE_CHANNEL, 0); // Apagar rojo
  delay(1000);
}

void testOff()
{

  ledcWrite(BLUE_CHANNEL, 255);  // Apagar rojo
  ledcWrite(RED_CHANNEL, 255);   // Apagar rojo
  ledcWrite(GREEN_CHANNEL, 255); // Apagar rojo
}

void sceneBluetoothDisconnected()
{

  ledcWrite(RED_CHANNEL, 255);   // Apagar rojo
  ledcWrite(GREEN_CHANNEL, 255); // Apagar rojo
  // Encender el LED azul con PWM al máximo brillo
  ledcWrite(BLUE_CHANNEL, 255);
  delay(200);

  // Apagar el LED azul
  ledcWrite(BLUE_CHANNEL, 0);
  delay(200);
}

void sceneGreenPlaying()
{
  ledcWrite(BLUE_CHANNEL, 255); // Apagar rojo
  ledcWrite(RED_CHANNEL, 255);  // Apagar rojo
  // Encender el LED verde
  ledcWrite(GREEN_CHANNEL, 0);
}

// Escena: Parpadeo rojo cuando la batería está baja
void sceneLowBattery()
{
  ledcWrite(BLUE_CHANNEL, 255);  // Apagar rojo
  ledcWrite(GREEN_CHANNEL, 255); // Apagar rojo
  // Encender el LED azul con PWM al máximo brillo
  ledcWrite(RED_CHANNEL, 255);
  delay(200);

  // Apagar el LED azul
  ledcWrite(RED_CHANNEL, 0);
  delay(200); // Pausa de 500 ms
}

// Escena: Amarillo fijo cuando el Bluetooth no está conectado o batería está bien

// Función para mapear el voltaje a porcentaje de batería
int mapBatteryToPercentage(float voltage)
{
  if (voltage >= 3.5)
    return 100; // 100% si el voltaje es 4.2V o mayor (batería llena)
  if (voltage <= 1.5)
    return 0; // 0% si el voltaje es 3.0V o menor (batería vacía)

  // Mapeo lineal entre 3.0V (0%) y 4.2V (100%)
  return (int)((voltage - 1.5) * 100 / (3.5 - 1.5));
}

// Función para leer el voltaje de la batería y convertirlo a porcentaje
int readBatteryPercentage()
{
  int rawValue = analogRead(BATTERY_PIN);    // Leer el valor analógico de la batería
  float voltage = rawValue * (3.3 / 4095.0); // Convertir a voltaje real
  Serial.printf("Battery Voltage: %.2f V\n", voltage);

  // Mapeo del voltaje a porcentaje de batería (ajusta los valores según tu configuración)
  int percentage = mapBatteryToPercentage(voltage);
  Serial.printf("Battery Percentage: %d %%\n", percentage);

  return percentage;
}

void setup()
{

  Serial.begin(115200);
  pinMode(RED_CHANNEL, OUTPUT);
  pinMode(BLUE_CHANNEL, OUTPUT);
  pinMode(GREEN_CHANNEL, OUTPUT);
  // Configuración de los canales PWM para cada LED
  ledcSetup(RED_CHANNEL, PWM_FREQ, PWM_RES);
  ledcSetup(GREEN_CHANNEL, PWM_FREQ, PWM_RES);
  ledcSetup(BLUE_CHANNEL, PWM_FREQ, PWM_RES);

  // Asociar los canales PWM a los pines correspondientes
  ledcAttachPin(RED_PIN, RED_CHANNEL);
  ledcAttachPin(GREEN_PIN, GREEN_CHANNEL);
  ledcAttachPin(BLUE_PIN, BLUE_CHANNEL);
  testOff();
  // Escena inicial
  sceneBluetoothDisconnected();
  // Setup Buttons
  for (byte currentPinIndex = 0; currentPinIndex < numOfButtons; currentPinIndex++)
  {
    pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);
    previousButtonStates[currentPinIndex] = HIGH;
    currentButtonStates[currentPinIndex] = HIGH;
  }

  // Setup Hat Switches
  for (byte currentPinIndex = 0; currentPinIndex < numOfHats * 4; currentPinIndex++)
  {
    pinMode(hatPins[currentPinIndex], INPUT_PULLUP);
    previousHatStates[currentPinIndex] = HIGH;
    currentHatStates[currentPinIndex] = HIGH;
  }

  BleGamepadConfiguration bleGamepadConfig;
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setWhichAxes(0, 0, 0, 0, 0, 0, 0, 0); // Disable all axes
  bleGamepadConfig.setButtonCount(numOfButtons);
  bleGamepadConfig.setHatSwitchCount(numOfHats);
  bleGamepad.begin(&bleGamepadConfig);

  sceneGreenPlaying();
  // changing bleGamepadConfig after the begin function has no effect, unless you call the begin function again
}

void loop()
{

  if (bleGamepad.isConnected())
  {
    int batteryPercentage = readBatteryPercentage();
    if (batteryPercentage < 50)
    {
      sceneLowBattery(); // Escena roja si la batería está baja
    }
    else
    {

      sceneGreenPlaying();
    }
    // Deal with buttons
    for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
    {
      currentButtonStates[currentIndex] = digitalRead(buttonPins[currentIndex]);

      if (currentButtonStates[currentIndex] != previousButtonStates[currentIndex])
      {
        if (currentButtonStates[currentIndex] == LOW)
        {
          bleGamepad.press(physicalButtons[currentIndex]);
        }
        else
        {
          bleGamepad.release(physicalButtons[currentIndex]);
        }
      }
    }

    // Update hat switch pin states
    for (byte currentHatPinsIndex = 0; currentHatPinsIndex < numOfHats * 4; currentHatPinsIndex++)
    {
      currentHatStates[currentHatPinsIndex] = digitalRead(hatPins[currentHatPinsIndex]);
    }

    // Update hats
    signed char hatValues[4] = {0, 0, 0, 0};

    for (byte currentHatIndex = 0; currentHatIndex < numOfHats; currentHatIndex++)
    {
      signed char hatValueToSend = 0;

      for (byte currentHatPin = 0; currentHatPin < 4; currentHatPin++)
      {
        // Check for direction
        if (currentHatStates[currentHatPin + currentHatIndex * 4] == LOW)
        {
          hatValueToSend = currentHatPin * 2 + 1;

          // Account for last diagonal
          if (currentHatPin == 0)
          {
            if (currentHatStates[currentHatIndex * 4 + 3] == LOW)
            {
              hatValueToSend = 8;
              break;
            }
          }

          // Account for first 3 diagonals
          if (currentHatPin < 3)
          {
            if (currentHatStates[currentHatPin + currentHatIndex * 4 + 1] == LOW)
            {
              hatValueToSend += 1;
              break;
            }
          }
        }
      }

      hatValues[currentHatIndex] = hatValueToSend;
    }

    // Set hat values
    bleGamepad.setHats(hatValues[0], hatValues[1], hatValues[2], hatValues[3]);

    // Update previous states to current states and send report
    if (currentButtonStates != previousButtonStates || currentHatStates != previousHatStates)
    {
      for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
      {
        previousButtonStates[currentIndex] = currentButtonStates[currentIndex];
      }

      for (byte currentIndex = 0; currentIndex < numOfHats * 4; currentIndex++)
      {
        previousHatStates[currentIndex] = currentHatStates[currentIndex];
      }

      bleGamepad.sendReport(); // Send a report if any of the button states or hat directions have changed
    }

    delay(10); // Reduce for less latency
  }
  else
  {
    sceneBluetoothDisconnected();
  }
}

// #include <Adafruit_NeoPixel.h> // Para controlar tiras de LED NeoPixel
// #include <NimBLEDevice.h>
// #include <NimBLEUtils.h>
// #include <NimBLEServer.h>
// #include "NimBLEHIDDevice.h"
// #include "HIDTypes.h"
// #include "BleConnectionStatus.h"
// #include "BleGamepad.h"
// #include "BleGamepadConfiguration.h"
// #include <Arduino.h>
// #include <nvs_flash.h>

// #define HID_GAMEPAD 0x03C4
// const int RED_PIN = 33;
// const int GREEN_PIN = 26;
// const int BLUE_PIN = 32;
// const int BATTERY_PIN = 35; // Pin para medir la batería

// // Definición de pines para joystick y botones
// const int joystickPins[] = {5, 18, 19, 21}; // UP, DOWN, LEFT, RIGHT
// // const int buttonPins[] = {13, 12, 14, 27, 26, 25, 33, 32, 22, 23}; // Botones adicionales
// // const int buttonPins[] = {22, 32, 13 , 23, 33, 12, 26, 27, 25, 14}; // TALVES TENEMSO QUE RESOLDAR!!

// const int buttonPins[] = {22, 23, 17, 16, 4, 15, 12, 13, 14, 27}; // TALVES TENEMSO QUE RESOLDAR!!
// // Mapeo de botones a los valores estándar reconocidos por Android
// NimBLEHIDDevice *hid;
// NimBLECharacteristic *input;

// // Escena: Green
// void sceneGreenPlaying()
// {
//   pinMode(BLUE_PIN, INPUT);
//   pinMode(RED_PIN, INPUT);
//   analogWrite(RED_PIN, 0);  // Rojo a máximo
//   analogWrite(BLUE_PIN, 0); // Rojo a máximo
//   pinMode(GREEN_PIN, OUTPUT);
//   analogWrite(GREEN_PIN, 255); // Rojo a máximo
// }

// // Escena: Parpadeo rojo cuando la batería está baja
// void sceneLowBattery()
// {
//   analogWrite(BLUE_PIN, 0); // Rojo a máximo
//   pinMode(BLUE_PIN, INPUT);
//   pinMode(RED_PIN, OUTPUT);
//   analogWrite(RED_PIN, 255); // Rojo a máximo
//   delay(200);                // Pausa de 500 ms
//   analogWrite(RED_PIN, 0);   // Apagar todos los LEDs
//   delay(200);                // Pausa de 500 ms
// }

// // Escena: Amarillo fijo cuando el Bluetooth no está conectado o batería está bien
// void sceneBluetoothDisconnected()
// {
//   analogWrite(RED_PIN, 0); // Rojo a máximo
//   pinMode(RED_PIN, INPUT);
//   pinMode(BLUE_PIN, OUTPUT);
//   analogWrite(BLUE_PIN, 255); // Rojo a máximo
//   delay(200);                 // Pausa de 500 ms
//   analogWrite(BLUE_PIN, 0);   // Apagar todos los LEDs
//   delay(200);                 // Pausa de 500 ms
// }

// // Función para mapear el voltaje a porcentaje de batería
// int mapBatteryToPercentage(float voltage)
// {
//   if (voltage >= 3.5)
//     return 100; // 100% si el voltaje es 4.2V o mayor (batería llena)
//   if (voltage <= 1.5)
//     return 0; // 0% si el voltaje es 3.0V o menor (batería vacía)

//   // Mapeo lineal entre 3.0V (0%) y 4.2V (100%)
//   return (int)((voltage - 1.5) * 100 / (3.5 - 1.5));
// }

// // Función para leer el voltaje de la batería y convertirlo a porcentaje
// int readBatteryPercentage()
// {
//   int rawValue = analogRead(BATTERY_PIN);    // Leer el valor analógico de la batería
//   float voltage = rawValue * (3.3 / 4095.0); // Convertir a voltaje real
//   Serial.printf("Battery Voltage: %.2f V\n", voltage);

//   // Mapeo del voltaje a porcentaje de batería (ajusta los valores según tu configuración)
//   int percentage = mapBatteryToPercentage(voltage);
//   Serial.printf("Battery Percentage: %d %%\n", percentage);

//   return percentage;
// }

// void setup()
// {
//   Serial.begin(115200);
//   sceneBluetoothDisconnected();
//   uint8_t buttonMap[] = {
//       0x01, // Button 1 - A
//       0x02, // Button 2 - B
//       0x03, // Button 3 - X
//       0x04, // Button 4 - Y
//       0x05, // Button 5 - L1
//       0x06, // Button 6 - R1
//       0x07, // Button 7 - Select (Back)
//       0x08, // Button 8 - Start
//       0x09, // Button 9 - L3
//       0x0A  // Button 10 - R3
//   };

//   // Inicializa BLE
//   NimBLEDevice::init("DuoGamerPro2");
//   // Crea un servidor BLE
//   NimBLEServer *pServer = NimBLEDevice::createServer();

//   // Configura el dispositivo HID
//   hid = new NimBLEHIDDevice(pServer);
//   hid->manufacturer()->setValue("SimpleManufacturer");

//   // Descriptor HID modificado para Android
//   uint8_t hidReportDescriptor[] = {
//   0x05, 0x01,        // Usage Page (Generic Desktop)
//   0x09, 0x05,        // Usage (Gamepad)
//   0xA1, 0x01,        // Collection (Application)
//   0x05, 0x09,        // Usage Page (Button)
//   0x19, 0x01,        // Usage Minimum (Button 1)
//   0x29, 0x10,        // Usage Maximum (Button 16)
//   0x15, 0x00,        // Logical Minimum (0)
//   0x25, 0x01,        // Logical Maximum (1)
//   0x75, 0x01,        // Report Size (1)
//   0x95, 0x10,        // Report Count (16)
//   0x81, 0x02,        // Input (Data,Var,Abs)
//       0x75, 0x06, // REPORT_SIZE (6)
//       0x95, 0x01, // REPORT_COUNT (1)
//       0x81, 0x03, // INPUT (Cnst,Var,Abs)
//       0x05, 0x01, // USAGE_PAGE (Generic Desktop)
//       0x09, 0x30, // USAGE (X)
//       0x09, 0x31, // USAGE (Y)
//       0x15, 0x81, // LOGICAL_MINIMUM (-127)
//       0x25, 0x7F, // LOGICAL_MAXIMUM (127)
//       0x75, 0x08, // REPORT_SIZE (8)
//       0x95, 0x02, // REPORT_COUNT (2)
//       0x81, 0x02, // INPUT (Data,Var,Abs)
//       0xC0        // END_COLLECTION
//   };

//   // Asigna el descriptor HID
//   hid->reportMap(hidReportDescriptor, sizeof(hidReportDescriptor));
//   input = hid->inputReport(1);
//   hid->startServices();

//   // Configura la publicidad BLE
//   NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
//   pAdvertising->setAppearance(HID_GAMEPAD); // Apariencia como un Gamepad
//   pAdvertising->addServiceUUID(hid->hidService()->getUUID());
//   pAdvertising->start();

//   Serial.println("BLE Gamepad ready to connect");

//   // Configuración de pines del joystick
//   for (int i = 0; i < 4; i++)
//   {
//     pinMode(joystickPins[i], INPUT_PULLUP);
//   }

//   // Configuración de pines de los botones
//   for (int i = 0; i < 10; i++)
//   {
//     pinMode(buttonPins[i], INPUT_PULLUP);
//   }

//   Serial.println("BLE Gamepad buttons Config. correct");
//   NimBLEDevice::setPower(ESP_PWR_LVL_P4);          // Configurar la potencia de transmisión a máxima
//   pServer->getAdvertising()->setMinInterval(0x30); // 30 ms
//   pServer->getAdvertising()->setMaxInterval(0x60); // 96 ms

//   sceneGreenPlaying();
// }

// void loop()
// {
//   // Leer el porcentaje de batería
// //  int batteryPercentage = readBatteryPercentage();

//   // bool btConnected = NimBLEDevice::getServer()->getConnectedCount() > 0;

//   // if (btConnected)
//   // {
//   //   NimBLEDevice::getAdvertising()->stop(); // Detén la publicidad si hay conexión
//   // }
//   // else
//   // {
//   //   NimBLEDevice::getAdvertising()->start(); // Inicia publicidad si no hay conexión
//   // }

//   // // if (batteryPercentage < 20)
//   // // {
//   // //   //  sceneLowBattery(); // Escena roja si la batería está baja
//   // // }

//   // if (!btConnected)
//   // {
//   //   sceneBluetoothDisconnected(); // Escena amarilla si el Bluetooth no está conectado
//   // }
//   // else
//   // {
//   //   //   sceneGreenPlaying(); // Escena verde si el Bluetooth está conectado y la batería está alta
//   // }

//   int8_t xAxis = 0;
//   int8_t yAxis = 0;
//   uint16_t buttons = 0;

//   // Leer joystick
//   if (digitalRead(joystickPins[3]) == LOW)
//   {
//     xAxis = -127; // LEFT
//     Serial.println("Left");
//   }
//   if (digitalRead(joystickPins[2]) == LOW)
//   {
//     xAxis = 127; // RIGHT
//     Serial.println("Right");
//   }
//   if (digitalRead(joystickPins[0]) == LOW)
//   {
//     yAxis = -127; // UP
//     Serial.println("Up");
//   }
//   if (digitalRead(joystickPins[1]) == LOW)
//   {
//     yAxis = 127; // DOWN
//     Serial.println("Down");
//   }

//   // Leer botones
//   for (int i = 0; i < 10; i++)
//   {
//     if (digitalRead(buttonPins[i]) == LOW)
//     {
//       Serial.print("Button ");
//       Serial.println(i + 1);
//       buttons |= (1 << i); // Establecer el bit correspondiente al botón presionado
//     }
//   }

//   // Enviar reporte
//   uint8_t report[4];
//   report[0] = buttons & 0xFF;        // Botones 1-8
//   report[1] = (buttons >> 8) & 0xFF; // Botones 9-16
//   report[2] = xAxis;                 // Eje X
//   report[3] = yAxis;                 // Eje Y

//   // Imprimir el reporte en formato hexadecimal
//   // Serial.print("Report: ");
//   // for (int i = 0; i < 4; i++)
//   // {
//   //   if (report[i] < 0x10)
//   //     Serial.print("0"); // Añadir un cero inicial si es necesario
//   //   Serial.print(report[i], HEX);
//   //   Serial.print(" ");
//   // }
//   Serial.println();
//   input->setValue(report, sizeof(report));
//   input->notify();
//   delay(50);
// }

///  ARRIBA DE ESTO HARE LO DE LOS LEDS

// // Pines sin funciones especiales
// //const int buttonPinsPart1[] = {4, 5, 12, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
// const int buttonPinsPart1[] = {13,12,14,27,26,*25,*33,*32, 4,15,16,17,5,18,19,21   , 22 , 23}; //  {34, 35, 36, 39, 2, 1 , 3 // DAN PROBLEMAS
// const int numPinsPart1 = sizeof(buttonPinsPart1) / sizeof(buttonPinsPart1[0]);

// const int buttonPinsPart2[] = {34, 35, 36, 39, 15, 3, 2};
// const int numPinsPart2 = sizeof(buttonPinsPart2) / sizeof(buttonPinsPart2[0]);

// void setup() {
//   Serial.begin(115200);
//   Serial.println("Inicia prueba de pines.");

//   Serial.println("Configurando pines sin funciones especiales...");
//   for (int i = 0; i < numPinsPart1; i++) {
//     pinMode(buttonPinsPart1[i], INPUT_PULLUP);
//     Serial.print("Pin configurado: ");
//     Serial.println(buttonPinsPart1[i]);
//   }

//   delay(2000); // Esperar un poco antes de configurar la segunda parte

//   // Serial.println("Configurando pines restantes...");
//   // for (int i = 0; i < numPinsPart2; i++) {
//   //   pinMode(buttonPinsPart2[i], INPUT_PULLUP);
//   //   Serial.print("Pin configurado: ");
//   //   Serial.println(buttonPinsPart2[i]);
//   // }
// }

// void loop() {

//   for (int i = 0; i < numPinsPart1; i++) {
//     int buttonState = digitalRead(buttonPinsPart1[i]);
//     if (buttonState == LOW) {
//       delay(50); // Pequeño delay para manejar el rebote
//       if (digitalRead(buttonPinsPart1[i]) == LOW) { // Verificar si sigue presionado
//         Serial.print("Botón presionado en el pin (Parte 1): ");
//         Serial.println(buttonPinsPart1[i]);

//         // Esperar a que se suelte el botón
//         while (digitalRead(buttonPinsPart1[i]) == LOW) {
//           delay(10);
//         }

//         Serial.println("Botón soltado. Continuando...");
//       }
//     }
//   }

//// LO DE ARRIBA SON LAS PRUEBAS DE BOTONES

// for (int i = 0; i < numPinsPart2; i++) {
//   int buttonState = digitalRead(buttonPinsPart2[i]);
//   if (buttonState == LOW) {
//     delay(50); // Pequeño delay para manejar el rebote
//     if (digitalRead(buttonPinsPart2[i]) == LOW) { // Verificar si sigue presionado
//       Serial.print("Botón presionado en el pin (Parte 2): ");
//       Serial.println(buttonPinsPart2[i]);

//       // Esperar a que se suelte el botón
//       while (digitalRead(buttonPinsPart2[i]) == LOW) {
//         delay(10);
//       }

//       Serial.println("Botón soltado. Continuando...");
//     }
//   }
// }

//  Serial.println("sigo vivo ...");
//   delay(100);
// }

// // Definición de pines para joystick y botones
// const int joystickPins[] = {5, 18, 19, 21};                        // UP, DOWN, LEFT, RIGHT
// //const int buttonPins[] = {13, 12, 14, 27, 26, 25, 33, 32, 22, 23}; // Botones adicionales
// const int buttonPins[] = {22, 32, 13 , 23, 33, 12, 26, 27, 25, 14}; // TALVES TENEMSO QUE RESOLDAR!!

// // Mapeo de botones a los valores estándar reconocidos por Android

// NimBLEHIDDevice *hid;
// NimBLECharacteristic *input;

// void setup()
// {
//     Serial.begin(115200);

//  uint8_t buttonMap[] = {
//     0x01, // Button 1 - A
//     0x02, // Button 2 - B
//     0x03, // Button 3 - X
//     0x04, // Button 4 - Y
//     0x05, // Button 5 - L1
//     0x06, // Button 6 - R1
//     0x07, // Button 7 - Select (Back)
//     0x08, // Button 8 - Start
//     0x09, // Button 9 - L3
//     0x0A  // Button 10 - R3
// };

//     // Inicializa BLE
//     NimBLEDevice::init("TableroBN");

//     // Crea un servidor BLE
//     NimBLEServer *pServer = NimBLEDevice::createServer();

//     // Configura el dispositivo HID
//     hid = new NimBLEHIDDevice(pServer);
//     hid->manufacturer()->setValue("SimpleManufacturer");

//     // Descriptor HID modificado para Android
//     uint8_t hidReportDescriptor[] = {
//         0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
//         0x09, 0x05,  // USAGE (Game Pad)
//         0xA1, 0x01,  // COLLECTION (Application)
//         0x85, 0x01,  // REPORT_ID (1)
//         0x05, 0x09,  // USAGE_PAGE (Button)
//         0x19, 0x01,  // USAGE_MINIMUM (Button 1)
//         0x29, 0x0A,  // USAGE_MAXIMUM (Button 10)
//         0x15, 0x00,  // LOGICAL_MINIMUM (0)
//         0x25, 0x01,  // LOGICAL_MAXIMUM (1)
//         0x75, 0x01,  // REPORT_SIZE (1)
//         0x95, 0x0A,  // REPORT_COUNT (10)
//         0x81, 0x02,  // INPUT (Data,Var,Abs)
//         0x75, 0x06,  // REPORT_SIZE (6)
//         0x95, 0x01,  // REPORT_COUNT (1)
//         0x81, 0x03,  // INPUT (Cnst,Var,Abs)
//         0x05, 0x01,  // USAGE_PAGE (Generic Desktop)
//         0x09, 0x30,  // USAGE (X)
//         0x09, 0x31,  // USAGE (Y)
//         0x15, 0x81,  // LOGICAL_MINIMUM (-127)
//         0x25, 0x7F,  // LOGICAL_MAXIMUM (127)
//         0x75, 0x08,  // REPORT_SIZE (8)
//         0x95, 0x02,  // REPORT_COUNT (2)
//         0x81, 0x02,  // INPUT (Data,Var,Abs)
//         0xC0         // END_COLLECTION
//     };

//     // Asigna el descriptor HID
//     hid->reportMap(hidReportDescriptor, sizeof(hidReportDescriptor));
//     input = hid->inputReport(1);
//     hid->startServices();

//     // Configura la publicidad BLE
//     NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
//     pAdvertising->setAppearance(HID_GAMEPAD); // Apariencia como un Gamepad
//     pAdvertising->addServiceUUID(hid->hidService()->getUUID());
//     pAdvertising->start();

//     Serial.println("BLE Gamepad ready to connect");

//     // Configuración de pines del joystick
//     for (int i = 0; i < 4; i++)
//     {
//         pinMode(joystickPins[i], INPUT_PULLUP);
//     }

//     // Configuración de pines de los botones
//     for (int i = 0; i < 10; i++)
//     {
//         pinMode(buttonPins[i], INPUT_PULLUP);
//     }

//     Serial.println("BLE Gamepad buttons Config. correct");
// }

// void loop()
// {
//     int8_t xAxis = 0;
//     int8_t yAxis = 0;
//     uint16_t buttons = 0;

//     // Leer joystick
//     if (digitalRead(joystickPins[0]) == LOW)
//     {
//         xAxis = -127; // LEFT
//         Serial.println("Left");
//     }
//     if (digitalRead(joystickPins[3]) == LOW)
//     {
//         xAxis = 127; // RIGHT
//         Serial.println("Right");
//     }
//     if (digitalRead(joystickPins[2]) == LOW)
//     {
//         yAxis = -127; // UP
//         Serial.println("Up");
//     }
//     if (digitalRead(joystickPins[1]) == LOW)
//     {
//         yAxis = 127; // DOWN
//         Serial.println("Down");
//     }

//     // Leer botones
//     for (int i = 0; i < 10; i++)
//     {
//         if (digitalRead(buttonPins[i]) == LOW)
//         {
//             Serial.print("Button ");
//             Serial.println(i + 1);
//             buttons |= (1 << i); // Establecer el bit correspondiente al botón presionado
//         }
//     }

//     // Enviar reporte
//     uint8_t report[4];
//     report[0] = buttons & 0xFF;        // Botones 1-8
//     report[1] = (buttons >> 8) & 0xFF; // Botones 9-16
//     report[2] = xAxis;                 // Eje X
//     report[3] = yAxis;                 // Eje Y

//     // Imprimir el reporte en formato hexadecimal
//     Serial.print("Report: ");
//     for (int i = 0; i < 4; i++)
//     {
//         if (report[i] < 0x10)
//             Serial.print("0"); // Añadir un cero inicial si es necesario
//         Serial.print(report[i], HEX);
//         Serial.print(" ");
//     }
//     Serial.println();

//     input->setValue(report, sizeof(report));
//     input->notify();

//     delay(10);
//     loopBattery();
// }
