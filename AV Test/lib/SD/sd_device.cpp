#include "sd_device.h"

// Variáveis globais no escopo deste arquivo para
// gerenciar o arquivo de log.
File data;
char file_name[20];

bool init_sd(uint8_t pin) {
    // Tenta inicializar o cartão SD no pino CS especificado.
    if (!SD.begin(pin)) {
        return false;
    }

    // Abre o diretório raiz do cartão SD.
    File root = SD.open("/");
    int countFilesOnSD = 0;

    // Itera por todos os arquivos no diretório raiz para contar quantos
    // arquivos de log já existem e determinar o nome do próximo.
    for (;;) {
        File entry = root.openNextFile();
        if (!entry) {
            // Não há mais arquivos para ler.
            break;
        }
        // Ignora diretórios.
        if (!entry.isDirectory()) {
            countFilesOnSD++;
        }
        entry.close();
    }

    // Cria o nome do novo arquivo de log com um número sequencial.
    sprintf(file_name, "/%s%d.csv", "AV_data", countFilesOnSD);

    // Tenta abrir o novo arquivo para escrita (append).
    data = SD.open(file_name, FILE_APPEND);

    if (data) {
        // Se o arquivo foi criado com sucesso, escreve o cabeçalho do CSV.
        data.println("tempo_30,tempo_100,velocidade");
        data.close();
        return true;
    } else {
        // Se não foi possível criar o arquivo.
        return false;
    }
}

bool save_AV_Data(unsigned long _t30, unsigned long _t100, float _v) {
    // Abre o arquivo de log já existente no modo de acréscimo (append).
    data = SD.open(file_name, FILE_APPEND);

    if (data) {
        // Escreve os dados da corrida formatados como uma linha CSV.
        data.printf("%d,%d,%f\r\n", _t30, _t100, _v);
        data.close();
        return true;
    }
    return false;
}