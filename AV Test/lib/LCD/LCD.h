#ifndef LCD_H
#define LCD_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// O tipo da função de callback para salvar no SD permanece o mesmo
typedef bool (*function_Sdsave)(unsigned long, unsigned long, float);

//== Funções de Inicialização e Status ==//
void init_lcd(void);
void error_message(void);
void ok_message(void);
void SD_status(bool status);
void display_run_error(const char* message);

//== Funções de Telas de UI ==//
void intro_text(void);
void select_sd(uint8_t pos1);

// --- FUNÇÕES CORRIGIDAS E RENOMEADAS ---
// Exibe a mensagem de que está salvando e o resultado (OK/Falhou)
void display_save_result(function_Sdsave sv_dt, unsigned long t30, unsigned long t100, float v);
// Exibe a mensagem "Voltando..." ao cancelar ou após salvar
void display_returning_message(void);

//== Funções de Exibição de Dados da Corrida ==//
void start_the_av_run(void);
void save_tcurr_time(unsigned long t);
void printRun(unsigned long t1, unsigned long t2);
void printRun(unsigned long t1);
void printRun(void);
String format_time(unsigned long int t1);
void save_t30(unsigned long t);
void save_t100(unsigned long t);
void save_speed(unsigned long t1, unsigned long t2);

#endif // LCD_H