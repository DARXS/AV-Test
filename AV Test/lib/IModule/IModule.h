#ifndef IMODULE_H
#define IMODULE_H

/**
 * @class IModule
 * @brief Interface (classe base abstrata) para todos os módulos do sistema.
 *
 * Define a estrutura essencial que cada módulo de hardware (0m, 30m, 100m, Bridge)
 * deve implementar. Isso permite que o código principal (`main.cpp`) gerencie
 * qualquer módulo de forma genérica, sem conhecer seus detalhes específicos.
 */
class IModule {
public:
    // Destrutor virtual para garantir a limpeza correta em classes derivadas.
    virtual ~IModule() {}

    /**
     * @brief Configura o hardware e inicializa o estado do módulo.
     *
     * Este método é chamado uma vez, dentro do `setup()` principal do Arduino.
     * Deve conter a inicialização de pinos, bibliotecas (LCD, SD, ESP-NOW) e
     * o estado inicial do módulo.
     */
    virtual void setup() = 0;

    /**
     * @brief Executa a lógica principal do módulo em um loop contínuo.
     *
     * Este método é chamado repetidamente dentro do `loop()` principal do Arduino.
     * Ele contém a máquina de estados e toda a lógica de operação do módulo.
     */
    virtual void loop() = 0;
};

#endif // IMODULE_H