
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
