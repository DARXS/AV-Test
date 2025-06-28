#include <Arduino.h>
#include "IModule.h"

// Includes para cada classe de módulo
#include "Module0m.h"
#include "Module30m.h"
#include "Module100m.h"
#include "BridgeModule.h"

// 'enum class' é uma forma mais moderna e segura de definir identificadores.
// Ele evita conflitos com outros números no código e torna a intenção mais clara.
enum class ModuleID : uint8_t {
    ECU,          // Módulo 0m
    Sensor30m,    // Módulo 30m
    Sensor100m,   // Módulo 100m
    Bridge        // Módulo Ponte
};
// ----------------------------------------------------------------

/*
 * Selecione o modo de operação alterando o valor de MODE.
 * Use um dos valores definidos no 'enum class ModuleID' acima.
 * Ex: ModuleID::ECU, ModuleID::Sensor30m, etc.
 */
#define MODE ModuleID::Sensor100m

// Ponteiro para a interface do módulo.
// Ele apontará para a instância do módulo ativo.
IModule* module = nullptr;

void setup() {
    Serial.begin(115200);

    switch (MODE) {
        case ModuleID::ECU:
            module = new Module0m();
            break;
        case ModuleID::Sensor30m:
            module = new Module30m();
            break;
        case ModuleID::Sensor100m:
            module = new Module100m();
            break;
        case ModuleID::Bridge:
            module = new BridgeModule();
            break;
    }

    if (module) {
        module->setup();
    } else {
        // Se o modo for inválido ou não tratado, entra em loop de erro.
        pinMode(LED_BUILTIN, OUTPUT);
        for (;;) {
            Serial.println("ERRO: MODO DE MODULO INVALIDO OU NAO TRATADO!");
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(250);
        }
    }
}

void loop() {
    if (module) {
        module->loop();
    }
}