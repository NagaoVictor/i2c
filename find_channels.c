#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

void write_reg(int fd, unsigned char reg, unsigned char val) {
    unsigned char buf[2] = {reg, val};
    write(fd, buf, 2);
}

void set_pwm(int fd, int ch, int on, int off) {
    int base = 0x06 + 4 * ch;
    unsigned char buf[5] = {
        base,
        on & 0xFF, (on >> 8) & 0xFF,
        off & 0xFF, (off >> 8) & 0xFF
    };
    write(fd, buf, 5);
}

int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0 || ioctl(fd, I2C_SLAVE, PCA9685_ADDR) < 0) {
        perror("[!] Erro ao abrir I2C");
        return 1;
    }

    // Reset e Configuração do PCA9685
    write_reg(fd, 0x00, 0x00);
    usleep(1000);
    write_reg(fd, 0x00, 0x10); // SLEEP
    usleep(5000);
    write_reg(fd, 0xFE, 121);  // 50Hz para Servos
    write_reg(fd, 0x00, 0xA0); // Restart + Auto-incremento
    usleep(10000);

    printf("====================================================\n");
    printf("   IDENTIFICADOR DE CANAL FISICO DO SERVO\n");
    printf("====================================================\n");
    printf("Procurando em qual pino o servo esta conectado...\n\n");

    for (int ch = 0; ch < 16; ch++) {
        printf("[--->] TESTANDO CANAL %d...\n", ch);

        // Move para a posição A (180 ticks)
        set_pwm(fd, ch, 0, 180);
        usleep(400000); // 400ms

        // Move para a posição B (420 ticks)
        set_pwm(fd, ch, 0, 420);
        usleep(400000);

        // Retorna ao centro (312 ticks)
        set_pwm(fd, ch, 0, 312);
        usleep(200000);
    }

    printf("\n[+] Varredura concluida em todos os 16 canais.\n");
    close(fd);
    return 0;
}