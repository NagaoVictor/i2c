#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40
#define CH_TILT 8  // Canal correto para Cima/Baixo

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
        perror("[!] Erro I2C");
        return 1;
    }

    // Inicializa PCA9685 a 50Hz
    write_reg(fd, 0x00, 0x00);
    usleep(1000);
    write_reg(fd, 0x00, 0x10);
    usleep(5000);
    write_reg(fd, 0xFE, 121);
    write_reg(fd, 0x00, 0xA0);
    usleep(10000);

    printf("====================================================\n");
    printf("   CALIBRADOR DE MIRA DO TILT (CANAL 8)\n");
    printf("====================================================\n");
    printf("Digite o valor em Ticks (ex: 200 a 400). Digite 0 para sair.\n\n");

    int ticks = 300; 
    while (1) {
        printf("Digite os ticks para o Tilt [Canal 8] (Atual: %d): ", ticks);
        if (scanf("%d", &ticks) != 1) break;
        if (ticks == 0) break;

        // Travas de segurança para o teste
        if (ticks < 150) ticks = 150;
        if (ticks > 450) ticks = 450;

        set_pwm(fd, CH_TILT, 0, ticks);
        printf("[+] Enviado %d ticks para o Canal 8 (Tilt).\n\n", ticks);
    }

    close(fd);
    printf("Saindo...\n");
    return 0;
}