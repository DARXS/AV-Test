# Projeto de Equipamento para Teste de Aceleração e Velocidade Veicular (AV Test)

Este repositório contém o código-fonte para um sistema de cronometragem de teste de aceleração de veículos, utilizando microcontroladores ESP32 e o protocolo de comunicação ESP-NOW.

## Visão Geral

O projeto consiste em um sistema modular projetado para medir com precisão os tempos de passagem de um veículo em distâncias predefinidas (30m e 100m) e calcular sua velocidade. O sistema é composto por uma Unidade de Controle Eletrônico (ECU) principal e módulos de sensores remotos.

## Estrutura de Arquivos

O projeto está organizado da seguinte forma:

```
AV Test/
├── backup/              # Arquivos de backup de versões anteriores
├── include/             # Arquivos de cabeçalho globais
│   ├── hardware_defs.h  # Definições de pinos de hardware
│   └── packets.h        # Estruturas de dados e enums de comunicação
├── lib/                 # Bibliotecas de código fonte dos módulos
│   ├── AV_espnow/       # Funções de wrapper para a comunicação ESP-NOW
│   ├── Bridge/          # Módulo ponte (repetidor de sinal)
│   ├── IModule/         # Interface base para todos os módulos
│   ├── LCD/             # Funções de controle do display LCD
│   ├── Module0m/        # Módulo ECU (0 metros)
│   ├── Module30m/       # Módulo sensor de 30 metros
│   ├── Module100m/      # Módulo sensor de 100 metros
│   └── SD/              # Funções para o cartão SD
├── src/                 # Arquivo principal do código fonte
│   └── main.cpp         # Ponto de entrada do programa
├── README.md            # Este arquivo
└── platformio.ini       # Arquivo de configuração do PlatformIO
```
- **`backup/`**: Contém versões antigas e código de rascunho que não fazem parte da compilação final.
- **`include/`**: Armazena definições e estruturas que são compartilhadas por todo o projeto.
    - `hardware_defs.h`: Mapeia os pinos do ESP32 para cada componente de hardware, como sensores, botões e o módulo SD.
    - `packets.h`: Define as estruturas de dados para a comunicação via ESP-NOW, os comandos da máquina de estados e os diferentes tipos de módulos.
- **`lib/`**: Contém as bibliotecas de código para cada componente funcional do projeto. Cada subdiretório representa um módulo ou funcionalidade específica.
    - `AV_espnow/`: Abstrai as funções do ESP-NOW para simplificar o envio e recebimento de dados.
    - `Bridge/`: Implementa um módulo repetidor que retransmite pacotes para estender o alcance da rede.
    - `IModule/`: Define uma interface (classe base abstrata) que padroniza a estrutura dos módulos, garantindo que todos tenham os métodos `setup()` e `loop()`.
    - `LCD/`: Gerencia o display LCD I2C, incluindo a exibição de menus, tempos de corrida e mensagens de status.
    - `Module0m/`: Código da ECU, responsável por coordenar a corrida, exibir os dados no LCD e salvar os resultados no cartão SD.
    - `Module30m/` e `Module100m/`: Código dos módulos sensores, responsáveis por detectar a passagem do veículo e enviar os tempos para a ECU. O módulo de 30m também atua como ponte para o de 100m.
    - `SD/`: Funções para inicializar o cartão SD e salvar os dados da corrida em formato CSV.
- **`src/main.cpp`**: É o coração do programa. Ele lê a configuração `MODE` para determinar qual módulo será instanciado e executado (ECU, Sensor 30m, Sensor 100m ou Bridge).
- **`platformio.ini`**: Arquivo de configuração do ambiente de desenvolvimento PlatformIO, que define a placa, o framework e as dependências do projeto.

## Funcionalidades

### Comunicação

- **Protocolo ESP-NOW**: O sistema utiliza o protocolo de comunicação sem fio ESP-NOW da Espressif, que permite a troca de dados de forma rápida e eficiente entre os dispositivos ESP32 sem a necessidade de uma rede Wi-Fi tradicional.
- **Estrutura de Pacotes**: A comunicação é padronizada através da estrutura `av_packet_t`, que inclui a identificação do módulo, um comando para a máquina de estados, o endereço MAC do remetente e os tempos medidos.
- **Módulo Ponte (Bridge)**: Para garantir a comunicação em distâncias maiores, um módulo `Bridge` pode ser utilizado para retransmitir os pacotes entre a ECU e os sensores mais distantes.

### Módulos

O sistema é dividido em diferentes tipos de módulos, selecionáveis no arquivo `src/main.cpp`:

1.  **`ModuleID::ECU` (`Module0m`)**: A unidade central.
    - Inicia e cancela as corridas.
    - Recebe os tempos dos módulos de 30m e 100m.
    - Exibe os tempos e a velocidade em um display LCD.
    - Salva os dados da corrida em um cartão SD.
    - Possui uma interface de usuário com botões e um potenciômetro para navegação.

2.  **`ModuleID::Sensor30m` (`Module30m`)**:
    - Detecta a passagem do veículo no marco de 30 metros usando um sensor.
    - Envia o tempo registrado para a ECU.
    - Atua como um repetidor, retransmitindo mensagens entre a ECU e o módulo de 100m.

3.  **`ModuleID::Sensor100m` (`Module100m`)**:
    - Detecta a passagem do veículo nos marcos de 100 e 101 metros.
    - Envia os dois tempos registrados para a ECU, permitindo o cálculo da velocidade final.

4.  **`ModuleID::Bridge`**:
    - Um módulo dedicado a retransmitir todos os pacotes ESP-NOW que recebe para aumentar o alcance e a robustez da comunicação.

### Máquina de Estados

Cada módulo opera com base em uma máquina de estados para controlar seu comportamento:

- **ECU (`av_ecu_t`)**: `initializing`, `menu`, `__start_run__`, `wait_to_start`, `lcd_display`, `end_run`, `save_run`.
- **Sensores (`state_t`)**: `wait`, `__setup__`, `run`.
