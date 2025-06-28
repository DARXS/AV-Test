#include "LCD.h"

//==================================================================
// Variáveis de Estado Interno da Biblioteca LCD
//==================================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Estas variáveis ainda são usadas para exibir os dados no LCD em tempo real
volatile unsigned long int t_30 = 0, t_100 = 0, t_101 = 0;
float vel = 0;
unsigned long int t_curr = 0;

String str_30 = "00:000",
       str_100 = "00:000",
       str_vel = "00.00 km/h";


//==================================================================
// Funções de Inicialização e Status
//==================================================================

void init_lcd() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.print(F("Iniciando..."));
    delay(500);
}

void error_message() {
    lcd.clear();
    lcd.print("ESP-NOW ERROR");
    delay(2000);
    lcd.clear();
    lcd.print("Reiniciando");
    delay(1000); lcd.print('.');
    delay(1000); lcd.print('.');
    delay(1000); lcd.print('.');
}

void ok_message() {
    lcd.clear();
    lcd.print(F("ESP-NOW ok!"));
    delay(600);
    lcd.clear();
}

void SD_status(bool status) {
    lcd.clear();
    String n = status ? "SD instalado" : "Nao ha SD";
    lcd.print(n);
    delay(120 * 7);
    lcd.clear();
}

void display_run_error(const char* message) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("!!! ERRO !!!"));
    lcd.setCursor(0, 1);
    lcd.print(message);
    delay(3000);
}


//==================================================================
// Funções de Telas de UI (Interface do Usuário)
//==================================================================

void intro_text() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("MANGUE AV - 4x4"));
    lcd.setCursor(0, 1);
    lcd.print(F("   RUN"));
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.write('>');
}

void select_sd(uint8_t pos1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(" DESEJA SALVAR? "));
    lcd.setCursor(0, 1);
    lcd.print(F("   SIM    NAO   "));
    lcd.setCursor(pos1 == 0 ? 1 : 8, 1);
    lcd.write('>');
}

// --- IMPLEMENTAÇÃO DAS FUNÇÕES CORRIGIDAS ---

// Exibe a mensagem "Voltando..."
void display_returning_message() {
    lcd.clear();
    lcd.print(F("Voltando..."));
    delay(120 * 7);
    lcd.clear();
}

// Exibe o resultado do salvamento e depois chama a mensagem de "Voltando..."
void display_save_result(function_Sdsave sv_dt, unsigned long t30, unsigned long t100, float v) {
    lcd.clear();
    lcd.print(F("Salvando: "));
    
    bool sd_save_ok = sv_dt(t30, t100, v);
    delay(1000);

    if (sd_save_ok) {
        lcd.setCursor(10, 0);
        lcd.print(F("OK"));
    } else {
        lcd.setCursor(0, 1);
        lcd.print(F("Falhou"));
    }
    delay(1200);
    
    // Chama a outra função para finalizar
    display_returning_message();
}


//==================================================================
// Funções de Exibição de Dados da Corrida
//==================================================================

void start_the_av_run() {
    t_30 = 0;
    t_100 = 0;
    t_101 = 0;
    vel = 0;
    str_vel = "00.00 km/h";
    printRun();
}

void save_tcurr_time(unsigned long t) {
    t_curr = t;
}

void printRun() {
    str_30 = format_time(t_30);
    str_100 = format_time(t_100);

    lcd.setCursor(0, 0);
    lcd.print(' ' + str_30 + "  " + str_100 + "    ");
    lcd.setCursor(0, 1);
    lcd.print("   " + str_vel + "        ");
}

void printRun(unsigned long t1, unsigned long t2) {
    t_30 = t1 - t_curr;
    t_100 = t2 - t_curr;
    printRun();
}

void printRun(unsigned long t1) {
    t_100 = t1 - t_curr;
    printRun();
}

String format_time(unsigned long int t1) {
    if (t1 >= 60000) return "xx:xxx";
    if (t1 < 10000)
        return '0' + String(t1 / 1000) + ':' + String(t1 % 1000);
    else
        return String(t1 / 1000) + ':' + String(t1 % 1000);
}

void save_t30(unsigned long t) {
    t_30 = t;
    printRun();
}

void save_t100(unsigned long t) {
    t_100 = t;
    printRun();
}

void save_speed(unsigned long t1, unsigned long t2) {
    if (vel != 0) {
        printRun();
        return;
    }

    if (t2 == 0 || t2 <= t1) {
        str_vel = "--.-- km/h";
    } else {
        float delta_t_s = (float)(t2 - t1) / 1000.0f;
        if (delta_t_s > 0) {
            float speed_mps = 1.0f / delta_t_s;
            vel = speed_mps * 3.6f;
            str_vel = String(vel, 2) + "km/h";
        } else {
            str_vel = "--.-- km/h";
        }
    }
    printRun();
}