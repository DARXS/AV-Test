#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include "soft_defs.h"
#include "hard_defs.h"

/* Defines type of ESP32 function */
#define M_0
//#define M_30
//#define M_100_101
//#define BRIDGE

typedef struct
{
  uint8_t ID;
  uint64_t first_msg;
  uint64_t second_msg;
} msg_packet;

msg_packet packet;

/* Libraries Variables */
LiquidCrystal_I2C lcd(0x27, 16, 2); 
esp_now_peer_info_t peerInfo;
File data;

/* Debug Variables */
bool interrupt = false;
bool sel = true, err = true;
unsigned long int curr = 0;
uint32_t ID_msg = 0x01;
uint64_t t0=0, t1=0;
/* Global Variables */
char file_name[20];
bool sd_exist = false;
uint8_t AddressFor_0[/*M_0 adress*/] = {0x40, 0x91, 0x51, 0xFB, 0xEA, 0x18}; // Each ESP32 have your Mac Adress

/* Interrupts Routine */
void ISR_30_100m();
/* Global Functions */
void Pin_Config();
bool Mount_SD();
void printRun();
String format_time(unsigned long int t1);
char potSelect(uint8_t pin, uint8_t num_options);
/* ESP-NOW Functions */
void receiveCallBack(const uint8_t* macAddr, const uint8_t* data, int len);
void sentCallBack(const uint8_t* macAddr, esp_now_send_status_t status);
void formatMacAddress(const uint8_t* macAddr, char* info, int maxLength);
bool sent_to_all(void *AnyMessage, int size);
bool sent_to_single(void *AnyMessage, int size);

void setup() 
{
  Serial.begin(115200);

  // Setup Pins
  Pin_Config();

  // Put ESP32 into Station Mode
  WiFi.mode(WIFI_MODE_STA);
  Serial.printf("Mac address: ");
  Serial.println(WiFi.macAddress());
  /*
    * E1_0 = 40:91:51:FB:EA:18
    * E2_30 = A8:42:E3:C8:47:48
    * E3_100 = EC:62:60:9C:BD:DC
  */

  // Disconnect from Wifi
  WiFi.disconnect();

  #ifdef M_0
    packet.ID = 0x00;
    packet.first_msg = 0;
    packet.second_msg = 0;

    lcd.init();
    lcd.backlight();
    lcd.clear();

    lcd.print(F("Iniciando..."));
  #endif
}

void loop() 
{
  switch(ss_t)
  {
    case INIT:
    {
      if(esp_now_init()==ESP_OK)
      {
        Serial.println("Init ESP_NOW Protocol");

        /* Broadcast Functions */ 
        esp_now_register_recv_cb(receiveCallBack);
        esp_now_register_send_cb(sentCallBack);

        #if defined(M_30) || defined(M_100_101)
          // Register peer
          memcpy(peerInfo.peer_addr, AddressFor_0, 6);
          peerInfo.channel = 0;
          peerInfo.encrypt = false;

          // Add peer
          if(esp_now_add_peer(&peerInfo)!=ESP_OK)
          {
            Serial.println("Failed to add peer");
            return;
          }

        #elif defined(BRIDGE)
          packet.ID = 0x80;
          packet.first_msg = 0;
          packet.second_msg = 0;
        #endif
      } 
      
      else 
      {

        #if defined(M_30) || defined(M_100_101) || defined(BRIDGE)
          digitalWrite(LED_BUILTIN, HIGH);
          delay(1500);
          esp_restart();

          break;
        #endif  

        //Serial.println("ESP-NOW init Failed!!");
        digitalWrite(LED_BUILTIN, HIGH);

        lcd.clear();
        lcd.print("ESP-NOW ERROR");
        delay(2000);
        lcd.clear();
        lcd.print("Reiniciando");
        delay(1000); lcd.print('.');
        delay(1000); lcd.print('.');
        delay(1000); lcd.print('.');

        delay(500);
        esp_restart(); 
      }

      #if defined(M_30) || defined(M_100_101)
        ss_t = WAIT;
      #elif defined(BRIDGE)
        ss_t = _BRIDGE_;  
      #else

        packet.ID |= ID_msg;
        do
        {
          sent_to_all(&packet, sizeof(msg_packet)); // send 1 how a flag
          delay(DEBOUCE_TIME);
          //Serial.printf("%d %d %d\n", esp_now_ok, conf_30, conf_100);
        } while(!esp_now_ok || !conf_30 || !conf_100);
        
        lcd.clear();
        lcd.print(F("ESP-NOW ok!"));
        delay(500);
        lcd.clear();

        ss_t = SD_BEGIN;
        //(sent_to_all(1) ? ss_t=WAIT : 0);
      #endif
      
      break;
    }
      
    case WAIT:
    {
      #ifndef M_0
        detachInterrupt(digitalPinToInterrupt(SENSOR_30_100));
        t_101 = 0;
        //vel = 0;
      #endif

      delay(50);
      break;
    }

    case SD_BEGIN:
    {
      (Mount_SD() ? sd_exist |= 0x01 : sd_exist &= ~0x01);

      lcd.print(F((sd_exist ? "SD Instalado" : "Nao ha SD")));
      delay(DEBOUCE_TIME*5);
      lcd.clear();

      ss_t = MENU;
      digitalWrite(LED_BUILTIN, LOW);

      break;
    }

    case MENU:
    {
      if(pot_sel!=old_pot)
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(F("MANGUE AV - 4x4"));
        lcd.setCursor(0,1);
        lcd.print(F("   RUN"));
        lcd.setCursor(0,1);
        lcd.print(" ");
        lcd.write('>');
        old_pot = pot_sel;
      }

      if(!digitalRead(B_SEL))
      {
        delay(DEBOUCE_TIME);
        while(!digitalRead(B_SEL));

        lcd.clear();
        t_30 = 0;
        t_100 = 0;
        t_101 = 0;
        vel = 0;
        t_curr = millis();
        
        old_pot = -1;
        ss_t = RUN;
      }

      //Serial.println(ss_t);
      break;
    }

    case RUN:
    {
      //Serial.println(ss_t);
      #ifdef M_0
        //if(!digitalRead(B_CAN))
        //{
        //  delay(DEBOUCE_TIME);
        //  while(!digitalRead(B_CAN));

        //  ss_t = MENU;
        //  ss_r = START_;
        //  old_pot = -1;
        //  sent_to_all(flag | 0x03);
        //}
      #else
        curr = millis();
      #endif

      switch(ss_r)
      {
        case START_:
        {
          t_30 = 0;
          t_100 = 0;
          t_101 = 0;
          vel = 0;
          str_vel = "00.00 km/h";
          printRun();

          //Serial.println(digitalRead(SENSOR_ZERO));
          if(digitalRead(SENSOR_ZERO))  
          {
            packet.ID = (ID_msg << 2);
            t0 = millis();
            sent_to_all(&packet, sizeof(msg_packet));
            t1 = millis();
            ss_r = LCD_DISPLAY;
            t_curr = millis();
          }

          break;
        }

        case LCD_DISPLAY:
        {
          while(ss_r==LCD_DISPLAY && sel /*|| err*/)
          {
            //(sel ? t_30 = millis() - t_curr : t_30);
            //(err ? t_100 = millis() - t_curr : 0);
            t_30 = millis() - t_curr;
            t_100 = millis() - t_curr;
            printRun();

            //if(!digitalRead(B_CAN))
            //{
            //  delay(DEBOUCE_TIME);
            //  while(!digitalRead(B_CAN));
            //  ss_t = MENU;
            //  ss_r = START_;
            //  old_pot = -1;
            //  sent_to_all(flag | 0x03);
            //}
          }
          printRun();
          //ss_r = END_RUN;

          while(ss_r==LCD_DISPLAY && err)
          {
            t_100 = millis() - t_curr;
            printRun();

            //if(!digitalRead(B_CAN))
            //{
            //  delay(DEBOUCE_TIME);
            //  while(!digitalRead(B_CAN));

            //  ss_t = MENU;
            //  ss_r = START_;
            //  old_pot = -1;
            //  sent_to_all(flag | 0x03);
            //}
          }

          //ss_r = END_RUN;
          while(ss_r!=END_RUN)
          {
            delay(DEBOUCE_TIME/15);
            //if(!digitalRead(B_SEL))
            //{
            //  delay(DEBOUCE_TIME);
            //  while(!digitalRead(B_SEL));
            //  ss_t = MENU;
            //  ss_r = START_;
            //  lcd.clear();
            //  delay(DEBOUCE_TIME);
            //}
          }
          printRun();

          break;
        }

        case WAIT_30:
        {
          attachInterrupt(digitalPinToInterrupt(SENSOR_30_100), ISR_30_100m, FALLING);
          while(ss_r==WAIT_30 && !interrupt)
          {
            packet.first_msg = millis() - curr;  
            //Serial.println(packet.first_msg);
          }
          
          while(!interrupt) delay(1);
          if(interrupt)
          {
            //Serial.println("hammm");
            packet.ID = 0x05;
            packet.second_msg = 0;
            sent_to_single(&packet, sizeof(packet));

            /* Reset the packet message */
            //packet.tt_30 = 0;
            //packet.flag &= ~0x05;
            interrupt = false;
            ss_t = WAIT;
            //ss_r = START_;
          }
          
          break;
        }

        case WAIT_100:
        {
          attachInterrupt(digitalPinToInterrupt(SENSOR_30_100), ISR_30_100m, FALLING);
          while(ss_r==WAIT_100 && !interrupt)
          {
            packet.first_msg = millis() - curr;
          }

          while(!interrupt) delay(1);
          if(interrupt)
          { 
            while(!digitalRead(SENSOR_101)) 
              packet.second_msg = millis() - curr;
            if(digitalRead(SENSOR_101)) 
              packet.second_msg = millis() - curr;

            packet.ID = 0x06;
            sent_to_single(&packet, sizeof(packet));

            t_101 = 0;
            interrupt = false;
            ss_t = WAIT;
            //ss_r = START_;
          }

          //packet.flag |= 0x06;
          //sent_to_single(packet);

          /* Reset the packet message */
          //packet.tt_100 = 0;
          //packet.tt_101 = 0;
          //packet.flag &= ~0x06;

          break;
        }

        case END_RUN:
        {
          if(vel==0)
          {
            vel = (double)((time_101-t_100)/1000.0);
            vel = (double)(3.6/vel);
            str_vel = String(vel, 2) + "km/h";
          }
          printRun();

          if(!digitalRead(B_SEL))
          {
            delay(DEBOUCE_TIME);
            while(!digitalRead(B_SEL));
            ss_r = SAVE_RUN;
            delay(100);
          }

          break;
        }

        case SAVE_RUN:
        {
          if(sd_exist)
          {
            pos[0] = 17; pos[1] = 24;
            pot_sel = potSelect(POT, 2);

            if(pot_sel!=old_pot)
            {
              lcd.setCursor(0, 0);
              lcd.print(F(" DESEJA SALVAR? "));
              lcd.setCursor(0, 1);
              lcd.print(F("   SIM    NAO   "));
              lcd.setCursor(pos[pot_sel]%16, (int)pos[pot_sel]/16);
              lcd.write('>');
              old_pot = pot_sel;
            }

            if(!digitalRead(B_SEL))
            {
              if(pot_sel==0)
              {
                lcd.print(F("  Salvando...  "));
                data = SD.open(file_name, FILE_APPEND);

                if(data)
                {
                  data.printf("%d,%d,%d\n", t_30, t_100, vel);
                  data.close();
                } 
              }

              else
              {
                lcd.print(F("Voltando"));
                delay(DEBOUCE_TIME*3);
                lcd.clear();
              }

              old_pot = -1;
              ss_t = MENU;
              ss_r = START_; 
              packet.ID = ID_msg | 0x02;
              sent_to_all(&packet, sizeof(msg_packet));
            }
          }

          else
          {
            lcd.print(F("Nao ha SD"));
            delay(DEBOUCE_TIME*3);
            lcd.clear();

            old_pot = -1;
            ss_t = MENU;
            ss_r = START_;
            packet.ID = ID_msg | 0x02;
            sent_to_all(&packet, sizeof(msg_packet));
          }

          break;
        }
      }
      
      break;
    }

    case _BRIDGE_:
    {
      delay(100);
    }
  }
}

/* Setup Functions */
void Pin_Config()
{
  pinMode(LED_BUILTIN, OUTPUT);
  #ifdef M_0
    pinMode(B_SEL, INPUT_PULLUP);
    //attachInterrupt(digitalPinToInterrupt(B_SEL), SelectISR, FALLING);
    //pinMode(B_CAN, INPUT_PULLUP);
    pinMode(B_CAN, INPUT);
    pinMode(SENSOR_ZERO, INPUT);
  #endif

  #ifdef M_30
    attachInterrupt(digitalPinToInterrupt(SENSOR_30_100), ISR_30_100m, FALLING);
  #endif

  #ifdef M_100_101
    attachInterrupt(digitalPinToInterrupt(SENSOR_30_100), ISR_30_100m, FALLING);
    pinMode(SENSOR_101, INPUT);
  #endif

  return;
}

/* ESP-NOW Functions */
void receiveCallBack(const uint8_t* macAddr, const uint8_t* data, int len)
{

  // Called when data is received
  msg_packet rec;
  //Serial.print("Data received: ");
  //Serial.println(len);
  //Serial.print("Message: ");
  //Serial.println(recv);
  //Serial.println();

  memcpy(&rec, data, sizeof(rec));
  if(rec.ID==0x05)
  {
    t_30 = rec.first_msg - ((t1-t0)*2);
    sel = false;
  }
    
  if(rec.ID==0x06)
  {
    t_100 = rec.first_msg - ((t1-t0)*2);
    t_101 = rec.second_msg - ((t1-t0)*2);
    err = false;
  }

  if(rec.ID == 0x80)
  {
    if(rec.first_msg==0x01)
    {
      #ifdef BRIDGE
        sent_to_all(&rec, sizeof(msg_packet));
      #else
        #ifdef M_30
          rec.first_msg ^= 0x01;
          sent_to_single(&rec, sizeof(msg_packet));;
          ss_t = WAIT;
          /* Reset flag */
          //packet.flag = 0x00;
        #endif

        #ifdef M_100_101
          rec.first_msg <<= 1;
          sent_to_single(&rec, sizeof(msg_packet));
          ss_t = WAIT;
          /* Reset Flag */
          //packet.flag &= ~(recv.flag << 1);
        #endif
      #endif
    }

    if(rec.first_msg==0x00)
    {
      #ifdef BRIDGE
        sent_to_all(&rec, sizeof(msg_packet));
      #else
        conf_30 = true; 
      #endif
    }

    if(rec.first_msg==0x02)
    {
      #ifdef BRIDGE
        sent_to_all(&rec, sizeof(msg_packet));
      #else
        conf_100 = true; 
      #endif
    }

    if(rec.first_msg==0x03)
    {
      #ifdef BRIDGE
        sent_to_all(&rec, sizeof(msg_packet));
      #else
        ss_t = WAIT;
        //ss_r = WAIT; 
      #endif
    }

    if(rec.first_msg==0x04)
    {
      #ifdef BRIDGE
        sent_to_all(&rec, sizeof(msg_packet));
      #else
        #ifdef M_30
          ss_t = RUN;
          ss_r = WAIT_30;
        #endif

        #ifdef M_100_101 
          t_101 = 0;
          ss_t = RUN;
          ss_r = WAIT_100;
        #endif
      #endif
    }
  }
}

void sentCallBack(const uint8_t* macAddr, esp_now_send_status_t status) 
{
  // Called when data is sent
  //char macStr[18];
  //formatMacAddress(macAddr, macStr, 18);

  //Serial.printf("Last packet sent to: %s\n", macStr);
  Serial.print("Last packet send status: ");
  Serial.println(status==ESP_NOW_SEND_SUCCESS ? esp_now_ok |= 0x01 : esp_now_ok &= ~0x01);

  (status==ESP_NOW_SEND_SUCCESS ? esp_now_ok |= 0x01 : esp_now_ok &= ~0x01);
}

void formatMacAddress(const uint8_t* macAddr, char* info, int maxLength)
{
  // Formats MAC Address
  snprintf(info, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

bool sent_to_all(void *AnyMessage, int size)
{
  // Broadcast a message to every device in range
  uint8_t BroadcastAdress[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  esp_now_peer_info_t BroadInfo = {};

  memcpy(&BroadInfo.peer_addr, BroadcastAdress, 6);

  if(!esp_now_is_peer_exist(BroadcastAdress)) { esp_now_add_peer(&BroadInfo); }

  // Send message
  esp_err_t result = esp_now_send(BroadcastAdress, (uint8_t *)AnyMessage, size);

  return result==ESP_OK ? true : false;
}

bool sent_to_single(void *AnyMessage, int size)
{
  esp_err_t result = esp_now_send(AddressFor_0, (uint8_t *)AnyMessage, size);

  return result==ESP_OK ? true : false;
}

/* Global Functions */
bool Mount_SD()
{
  if(!SD.begin(SD_CS)) { return false; }

  File root = SD.open("/");
  int CountFilesOnSD = 0;

  while(true)
  {
    File entry = root.openNextFile();
    // no more files
    if(!entry) { break; }

    // for each file count it
    CountFilesOnSD++;
    entry.close();
  }

  sprintf(file_name, "/%s%d.csv", "AV_data", CountFilesOnSD);

  data = SD.open(file_name, FILE_APPEND);

  if(data)
  {
    data.close();

    return true;
  }

  else
  {
    return false;
  }
}

void printRun()
{
  str_30 = format_time(t_30);
  str_100 = format_time(t_100);
  str_101 = format_time(t_101);

  lcd.setCursor(0,0);
  lcd.print(' ' + str_30 + "  " + str_100 + "    ");
  lcd.setCursor(0, 1);
  lcd.print("   " + str_vel + "        ");
}

String format_time(unsigned long int t1)
{
  if(t1 < 10000)
    return '0' + String(t1 / 1000) + ':' + String(t1 % 1000);
  else
    return String(t1 / 1000) + ':' + String(t1 % 1000);
}

char potSelect(uint8_t pin, uint8_t num_options)
{
  uint16_t read_val = analogRead(pin);
  uint8_t option = map(read_val, 50, 1000, 0, 2*num_options-1);
  //    if (option >= num_options)
  //        option -= num_options;
  return option % num_options;
}

/* Interrupts Routine */
void ISR_30_100m()
{
  if(ss_r==WAIT_30)
  {
    packet.first_msg = millis() - curr;
  }

  else
  {
    packet.first_msg = millis() - curr;
  }
  
  interrupt = true;
  detachInterrupt(digitalPinToInterrupt(SENSOR_30_100));
}