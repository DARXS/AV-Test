#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
/* User libraries */
#include "soft_defs.h"
#include "hard_defs.h"

/* Defines type of ESP32 function */
#define M_0
//#define M_30
//#define M_100_101

#ifdef M_0
  typedef struct
  {
    uint8_t message_Id;
    uint64_t current_time;
  } m_30_packet;

  typedef struct
  {
    uint8_t message_Id;
    uint64_t current_time_100;
    uint64_t time_in_101;
  } m_100_packet;

  m_30_packet pack_30m;
  m_100_packet pack_100m;

#endif

#ifdef M_30

  typedef struct
  {
    uint8_t message_Id = 0x05;
    uint64_t current_time = 0;
  } m_30_packet;

  m_30_packet packet_30m;

#endif

#ifdef M_100_101
  typedef struct
  {
    uint8_t message_Id = 0x06;
    uint64_t current_time_100 = 0;
    uint64_t time_in_101 = 0;
  } m_100_packet;

  m_100_packet packet_100m;
#endif

/* Libraries Variables */
LiquidCrystal_I2C lcd(0x27, 16, 2); 
esp_now_peer_info_t peerInfo;
File data;

/* Debug Variables */
bool interrupt = false;
uint64_t t = 0;
bool first_ac = true;
bool sel = true, err = true;
unsigned long int curr = 0;
/* Global Variables */
char file_name[20];
bool sd_exist = false;
//const uint8_t AddressFor_0[/*M_0 adress*/] = {0x40, 0x91, 0x51, 0xFB, 0xEA, 0x18}; 
//AVR_SENT AVR(AddressFor_0);
AVR_SENT avr(0x40, 0x91, 0x51, 0xFB, 0xEA, 0x18); // Each ESP32 have your Mac Adress

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

void setup() 
{
  Serial.begin(115200);

  //Serial.println(avr.formatMacAddress());

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
        //Serial.println("Init ESP_NOW Protocol");

        /* Broadcast Functions */ 
        esp_now_register_recv_cb(receiveCallBack);
        esp_now_register_send_cb(sentCallBack);

        #if defined(M_30) || defined(M_100_101)
          // Register peer
          int ss = avr.Register_Peer(peerInfo);
          
          if(ss==-1 || ss==0)
          {
            Serial.print("Add peer error!!!!");
            while(1);
          }
        #endif
      } 
      
      else 
      {
        #if defined(M_30) || defined(M_100_101)
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

        break;
      }

      #if defined(M_30) || defined(M_100_101)
        ss_t = WAIT;
        break;
      #else

        do
        {
          // Sent to All
          esp_now_ok = avr.Send_Byte(0x01);
          delay(DEBOUCE_TIME);
          //Serial.printf("%d %d %d\n", esp_now_ok, conf_30, conf_100);
        } while(!esp_now_ok || !conf_30 || !conf_100);
        
        lcd.clear();
        lcd.print(F("ESP-NOW ok!"));
        delay(500);
        lcd.clear();

        ss_t = SD_BEGIN;

        // Sent to all
        //(AVR.Sent_Data(1) ? ss_t=WAIT : 0);
      #endif
      
      break;
    }
      
    case WAIT:
    {
      #ifndef M_0
        detachInterrupt(digitalPinToInterrupt(SENSOR_30_100));
        //t_101 = 0;
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
        if(!digitalRead(B_CAN))
        {
          delay(DEBOUCE_TIME);
          while(!digitalRead(B_CAN));

          ss_t = MENU;
          ss_r = START_;
          old_pot = -1;
          // Sent to all
          avr.Send_Byte(0x03);
        }
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
            // Sent to all
            t = millis();
            avr.Send_Byte(0x04);
            ss_r = LCD_DISPLAY;
            t_curr = millis();
          }

          break;
        }

        case LCD_DISPLAY:
        {
          while(ss_r==LCD_DISPLAY && sel)
          {
            //(sel ? t_30 = millis() - t_curr : t_30);
            //(err ? t_100 = millis() - t_curr : 0);
            t_30 = millis() - t_curr;
            t_100 = millis() - t_curr;
            printRun();

            if(!digitalRead(B_CAN))
            {
              delay(DEBOUCE_TIME);
              while(!digitalRead(B_CAN));
              ss_t = MENU;
              ss_r = START_;
              old_pot = -1;
              // Sent to all
              avr.Send_Byte(0x03);
            }
          }
          printRun();
          //ss_r = END_RUN;

          while(ss_r==LCD_DISPLAY && err)
          {
            t_100 = millis() - t_curr;
            printRun();

            if(!digitalRead(B_CAN))
            {
              delay(DEBOUCE_TIME);
              while(!digitalRead(B_CAN))
              ss_t = MENU;
              ss_r = START_;
              old_pot = -1;
              // Sent to all
              avr.Send_Byte(0x03);
            }
          }

          //ss_r = END_RUN;
          while(ss_r!=END_RUN)
          {
            delay(DEBOUCE_TIME/10);
            if(!digitalRead(B_SEL))
            {
              delay(DEBOUCE_TIME);
              while(!digitalRead(B_SEL));
              ss_t = MENU;
              ss_r = START_;
              lcd.clear();
              delay(DEBOUCE_TIME);
            }
          }
          printRun();

          break;
        }

        case WAIT_30:
        {
          #ifdef M_30
            attachInterrupt(digitalPinToInterrupt(SENSOR_30_100), ISR_30_100m, FALLING);
            //while(ss_r==WAIT_30 && !interrupt && count<2)
            //{
            //  packet.tt_30 = millis() - curr;  
            //  Serial.println(packet.tt_30);
            //}
            
            while(!interrupt) delay(1);
            if(interrupt)
            {
              // Sent to single
              avr.Send_Data(&packet_30m, sizeof(m_30_packet), false);

              /* Reset the packet message */
              //packet.tt_30 = 0;
              //packet.flag &= ~0x05;
              interrupt = false;
              ss_t = WAIT;
              //ss_r = START_;
            }
          #endif
          
          break;
        }

        case WAIT_100:
        {
          #ifdef M_100_101
            attachInterrupt(digitalPinToInterrupt(SENSOR_30_100), ISR_30_100m, FALLING);
            //while(ss_r==WAIT_100 && !interrupt)
            //{
              //packet.tt_100 = millis() - curr;
            //}

            while(!interrupt) delay(1);
            if(interrupt)
            {
              // Sent to single
              //AVR.Sent_Data(flag | 0x06, peerInfo, 1);;  

              while(!digitalRead(SENSOR_101)) 
                packet_100m.time_in_101 = millis() - curr;
              if(digitalRead(SENSOR_101)) 
                packet_100m.time_in_101 = millis() - curr;

              //memcpy(&teste, (uint16_t *)&speed, sizeof(uint16_t));

              //while(t_101>0xff)
              //{
              //  // Sent to single
              //  //AVR.Sent_Data(0xff, peerInfo, 1);
              //  delay(DEBOUCE_TIME/10);
              //  t_101 -= 0xff;
              //}

              // Sent to single
              avr.Send_Data(&packet_100m, sizeof(m_100_packet), false);
              packet_100m.time_in_101 = 0;
              interrupt = false;
              ss_t = WAIT;
              //ss_r = START_;
            }
          #endif

          //packet.flag |= 0x06;
          // Sent to single
          //AVR.Sent_Data(packet, peerInfo, 1);;

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
            vel = (double)((t_101-t_100)/1000.0);
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
          first_ac = true;
          err = true;
          sel = true;

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
                  data.printf("%s,%s,%s\n", str_30, str_100, vel);
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
              // Sent to all
              avr.Send_Byte(0x03);
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
            // Sent to all
            avr.Send_Byte(0x03);
          }

          break;
        }
      }
      
      break;
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
void receiveCallBack(const uint8_t *macAddr, const uint8_t *data, int len)
{
  // Called when data is received
  uint8_t recv;
  
  //Serial.print("Data received: ");
  //Serial.println(len);
  //Serial.print("Message: ");
  //Serial.println(recv);
  //Serial.println();

  if(len==sizeof(uint8_t))
  {
    memcpy(&recv, (uint8_t *)&data, len);

    if(recv==0)
    {
      conf_30 = true;
    }

    else if(recv==1)
    {
      #ifdef M_30                                                                 
        avr.Send_Byte(recv ^ 0x01, false);
        ss_t = WAIT;
      #endif

      #ifdef M_100_101
        avr.Send_Byte(recv << 0x01, false);
        ss_t = WAIT;
      #endif
    }

    else if(recv==2)
    {
      conf_100 = true;
    }
    
    else if(recv==3)
    {
      ss_t = WAIT;
    }    

    else if(recv==4)
    {
      #ifdef M_30
        ss_t = RUN;
        ss_r = WAIT_30;
      #endif      

      #ifdef M_100_101
        t_101 = 0;
        ss_t = RUN;
        ss_r = WAIT_100;
      #endif
    }
  }

  else
  {
    #ifdef M_0
      if(first_ac)
      {
        memcpy(&pack_30m, (m_30_packet *)&recv, sizeof(m_30_packet));
        if(pack_30m.message_Id == 0x05)
        {
          t_30 = pack_30m.current_time - t*2;
          sel = false;
          first_ac = false;
        }
      }

      else
      {
        memcpy(&pack_100m, (m_100_packet *)&recv, sizeof(m_100_packet));
        if(pack_100m.message_Id==0x06)
        {
          t_100 = pack_100m.current_time_100 - t*2;
          t_101 = pack_100m.time_in_101 - t*2;

          err = false;
          ss_r = END_RUN;
        }
      }
    #endif
  }
  

  //  else if(recv[4]==5)
  //  {
  //    //t_30 = millis() - t_curr;
  //    //packet.tt_30 = recv.tt_30;
  //    //digitalWrite(LED_BUILTIN, HIGH);
  //    //Serial.println(packet.tt_30);
  //    //Serial.println((float)t_30/1000);
  //    sel = false;
  //  }

  //  else if(recv[5]==6)
  //  {
  //    //t_100 = recv.tt_100;
  //    //t_101 = recv.tt_101;
  //    //ss_r = END_RUN;
  //    err = false;
  //    ignore = true;
  //  }
  //}

  //else
  //{
  //  time_101 += recv[6];
  //  //Serial.println(time_101);

  //  if(recv[6]!=0xff)
  //  {
  //    ss_r = END_RUN;
  //    ignore = false;
  //  }    
  //}
}

void sentCallBack(const uint8_t* macAddr, esp_now_send_status_t status) 
{
  // Called when data is sent
  char macStr[18];

  //Serial.printf("Last packet sent to: %s\n", macStr);
  //Serial.print("Last packet send status: ");
  //Serial.println(status==ESP_NOW_SEND_SUCCESS ? esp_now_ok |= 0x01 : esp_now_ok &= ~0x01);

  (status==ESP_NOW_SEND_SUCCESS ? esp_now_ok |= 0x01 : esp_now_ok &= ~0x01);
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
    #ifdef M_30
      packet_30m.current_time = millis() - curr;
    #endif
  }

  else
  {
    #ifdef M_100_101
      packet_100m.current_time_100 = millis() - curr;
    #endif
  }
  
  interrupt = true;
  detachInterrupt(digitalPinToInterrupt(SENSOR_30_100));
}