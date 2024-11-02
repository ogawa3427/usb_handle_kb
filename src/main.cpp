// For M5Stack AtomS3

#include <Arduino.h>
#include <M5Unified.h>
#include "keys.h"

// #define USB_SERIAL (1)

// https://wiki.osdev.org/USB_Human_Interface_Devices#Detecting_HID_devices
// https://aeb.win.tue.nl/linux/kbd/scancodes-14.html
// https://lang-ship.com/blog/work/ch559/#toc9

bool is_shift_pressed = false;

enum OUTER_STATE
{
  MAIN,
  MEMO,
};

OUTER_STATE outerState;

std::vector<uint16_t> last_keyboard_data;

std::vector<uint16_t> additional;
std::vector<uint16_t> deletional;

std::vector<uint16_t> keys_stack;
uint16_t keys_stack_Top = 0x00;
uint16_t last_keys_stack_Top = 0x00;

void detection(String hexString, String receivedData)
{
  std::vector<uint16_t> keyboard_data;

  uint16_t devType = receivedData[4];
  uint16_t modKeysByte = receivedData[11];
  uint16_t ordinalKeysByte[6] = {receivedData[13], receivedData[14], receivedData[15], receivedData[16], receivedData[17], receivedData[18]};

#ifdef USB_SERIAL
  USBSerial.print("modKeysByte: ");
  USBSerial.println(modKeysByte);
  USBSerial.print("ordinalKeysByte1: ");
  USBSerial.println(ordinalKeysByte[0]);
  USBSerial.print("ordinalKeysByte2: ");
  USBSerial.println(ordinalKeysByte[1]);
  USBSerial.print("ordinalKeysByte3: ");
  USBSerial.println(ordinalKeysByte[2]);
  USBSerial.print("ordinalKeysByte4: ");
  USBSerial.println(ordinalKeysByte[3]);
  USBSerial.print("ordinalKeysByte5: ");
  USBSerial.println(ordinalKeysByte[4]);
  USBSerial.print("ordinalKeysByte6: ");
  USBSerial.println(ordinalKeysByte[5]);
#endif

  if (devType == 0x06)
  {
    // Mod Keys
    if (modKeysByte & 0b00000001)
    {
      keyboard_data.push_back(KEYBOARD_LEFT_CTRL);
    }
    if (modKeysByte & 0b00000010)
    {
      keyboard_data.push_back(KEYBOARD_LEFT_SHIFT);
    }
    if (modKeysByte & 0b00000100)
    {
      keyboard_data.push_back(KEYBOARD_LEFT_ALT);
    }
    if (modKeysByte & 0b00001000)
    {
      keyboard_data.push_back(KEYBOARD_LEFT_GUI);
    }
    if (modKeysByte & 0b00010000)
    {
      keyboard_data.push_back(KEYBOARD_RIGHT_CTRL);
    }
    if (modKeysByte & 0b00100000)
    {
      keyboard_data.push_back(KEYBOARD_RIGHT_SHIFT);
    }
    if (modKeysByte & 0b01000000)
    {
      keyboard_data.push_back(KEYBOARD_RIGHT_ALT);
    }
    if (modKeysByte & 0b10000000)
    {
      keyboard_data.push_back(KEYBOARD_RIGHT_GUI);
    }

    for (int i = 0; i < 6; i++)
    {
      if (ordinalKeysByte[i] != 0x00)
      {
        keyboard_data.push_back(ordinalKeysByte[i]);
      }
    }

    for (int i = 0; i < last_keyboard_data.size(); i++)
    {
      if (std::find(keyboard_data.begin(), keyboard_data.end(), last_keyboard_data[i]) == keyboard_data.end())
      {
        deletional.push_back(last_keyboard_data[i]);
      }
    }

    for (int i = 0; i < keyboard_data.size(); i++)
    {
      if (std::find(last_keyboard_data.begin(), last_keyboard_data.end(), keyboard_data[i]) == last_keyboard_data.end())
      {
        additional.push_back(keyboard_data[i]);
      }
    }

#ifdef USB_SERIAL
    for (int i = 0; i < keyboard_data.size(); i++)
    {
      USBSerial.printf("%02X ", keyboard_data[i]);
      USBSerial.print(" ");
      USBSerial.print(keyNameDict[keyboard_data[i]]);
      USBSerial.print(" ");
    }
    USBSerial.println();

    USBSerial.print("additional: ");
    for (int i = 0; i < additional.size(); i++)
    {
      USBSerial.printf("%02X ", additional[i]);
    }
    USBSerial.println();

    USBSerial.print("deletional: ");
    for (int i = 0; i < deletional.size(); i++)
    {
      USBSerial.printf("%02X ", deletional[i]);
    }
    USBSerial.println();
#endif
    for (int i = 0; i < additional.size(); i++)
    {
      keys_stack.insert(keys_stack.begin(), additional[i]);
    }

    for (int i = 0; i < deletional.size(); i++)
    {
      keys_stack.erase(std::remove(keys_stack.begin(), keys_stack.end(), deletional[i]), keys_stack.end());
    }

#ifdef USB_SERIAL
    for (int i = 0; i < keys_stack.size(); i++)
    {
      USBSerial.printf("%02X ", keys_stack[i]);
      USBSerial.print(" ");
      USBSerial.println(keyNameDict[keys_stack[i]]);
    }
    USBSerial.println();
#endif
    if (keys_stack.empty())
    {
      keys_stack_Top = 0x00;
    }
    else
    {
      keys_stack_Top = keys_stack[0];
    }

    last_keyboard_data = keyboard_data;
    // last_keys_stack_Top = keys_stack_Top;
    additional.clear();
    deletional.clear();
  }
}

void setup()
{
  // UARTを初期化
  Serial2.begin(115200, SERIAL_8N1, 5, 6);

#ifdef USB_SERIAL
  USBSerial.begin(115200);
#endif

  auto cfg = M5.config();
  M5.begin(cfg);
  // M5.Display.startWrite();
  // M5.Display.setCursor(0, 0);
  // M5.Display.print(millis());
  // M5.Display.endWrite();
  M5.Lcd.setTextColor(YELLOW);

  outerState = MEMO;
  M5.Lcd.setTextSize(3);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setBrightness(90);
  // USBSerial.println("Hello!");

  M5.Lcd.setTextSize(3);

}

String memo_text = "";

void loop()
{
  M5.update();
  String receivedData = "";
  String hexString = "";
  if (Serial2.available())
  {
    receivedData = Serial2.readStringUntil('\n');
    for (int i = 0; i < receivedData.length(); i++)
    {
      char hex[3];
      sprintf(hex, "%02X", receivedData[i]);
      hexString += hex;
    }
  }

  if (outerState == MAIN)
  {
    detection(hexString, receivedData);
    if (keys_stack_Top != last_keys_stack_Top)
    {
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.fillScreen(BLACK);
      if (keys_stack_Top != 0x00)
      {
        M5.Lcd.println(keyNameDict[keys_stack_Top]);
        // M5.Lcd.println(shiftedKeyNameDict[keys_stack_Top]);
        M5.Lcd.printf("%02X", keys_stack_Top);
      }
      last_keys_stack_Top = keys_stack_Top;
    }
  }
  else if (outerState == MEMO)
  {
    detection(hexString, receivedData);  // この行を追加

    if (keys_stack_Top == KEYBOARD_LEFT_SHIFT || keys_stack_Top == KEYBOARD_RIGHT_SHIFT)
    {
      is_shift_pressed = true;
    }

    if (is_shift_pressed){
      bool shift_found = false;
      for (size_t i = 0; i < keys_stack.size(); i++){
        if (keys_stack[i] == KEYBOARD_LEFT_SHIFT || keys_stack[i] == KEYBOARD_RIGHT_SHIFT){
          shift_found = true;
          break;
        }
      }
      if (!shift_found){
        is_shift_pressed = false;
      }
    }

    if (keys_stack_Top != 0x00 && keys_stack_Top != last_keys_stack_Top)
    {
      if ((keys_stack_Top >= 0x04 && keys_stack_Top <= 0x27) || (keys_stack_Top >= 0x2D && keys_stack_Top <= 0x2F) || (keys_stack_Top == 0x89) || (keys_stack_Top == 0x30) || (keys_stack_Top >= 0x32 && keys_stack_Top <= 0x34) || (keys_stack_Top >= 0x36 && keys_stack_Top <= 0x39))
      {
        if (is_shift_pressed)
        {
          memo_text += shiftedKeyNameDict[keys_stack_Top];
        }
        else
        {
          memo_text += keyNameDict[keys_stack_Top];
        }
      }
      else if (keys_stack_Top == 0x28)
      {
        memo_text += "\n";
      }
      else if (keys_stack_Top == 0x2C)
      {
        memo_text += " ";
      }
      else if (keys_stack_Top == 0x2A && !memo_text.isEmpty())
      {
        memo_text.remove(memo_text.length() - 1);
      }
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(3);
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.print(memo_text);
    }
    last_keys_stack_Top = keys_stack_Top;
  }

  // if (M5.BtnA.wasPressed())
  // {
  //   if (outerState == MAIN)
  //   {
  //     outerState = MEMO;
  //   }
  //   else if (outerState == MEMO)
  //   {
  //     outerState = MAIN;
  //   }
  // }
}
