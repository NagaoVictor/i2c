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

unsigned char read_reg(int fd, unsigned char reg) {
    unsigned char val;
    write(fd, &reg, 1);
    read(fd, &val, 1);
    return val;
}

// Envia comando PWM para TODOS os 16 canais simultaneamente usando o registrador ALL_LED
void set_all_pwm(int fd, int on, int off) {
    unsigned char buf[5] = {
        0xFA, // ALL_LED_ON_L
        on & 0xFF, (on >> 8) & 0xFF,
        off & 0xFF, (off >> 8) & 0xFF
    };
    write(fd, buf, 5);
}

int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0 || ioctl(fd, I2C_SLAVE, PCA9685_ADDR) < 0) {
        perror("[!] Erro ao acessar I2C");
        return 1;
    }

    printf("[+] Verificando estado interno do PCA9685...\n");
    
    // Reset Completo
    write_reg(fd, 0x00, 0x00);
    usleep(10000);

    // Configura Prescale para ~50Hz (Servos)
    // Oscillator 25MHz / (4096 * 50Hz) - 1 = 121
    write_reg(fd, 0x00, 0x10); // Entra em modo SLEEP
    usleep(5000);
    write_reg(fd, 0xFE, 121);  // Grava Prescale
    write_reg(fd, 0x00, 0xA0); // Acorda e liga Auto-Incremento
    usleep(10000);

    unsigned char mode1 = read_reg(fd, 0x00);
    unsigned char prescale = read_reg(fd, 0xFE);

    printf("[+] MODE1: 0x%02X (Esperado: 0xA0 ou 0x20)\n", mode1);
    printf("[+] PRESCALE: %d (Esperado: 121)\n", prescale);

    if (prescale != 121) {
        printf("[!] AVISO: O registrador PRESCALE não aceitou o valor. Verifique a alimentação VCC!\n");
    }

    printf("\n[+] Disparando pulso em TODOS OS 16 CANAIS simultaneamente...\n");

    for (int cycle = 0; cycle < 4; cycle++) {
        printf(" -> Posição 0 Graus (Pulso Curto ~100 ticks / 0.5ms)\n");
        set_all_pwm(fd, 0, 100);
        sleep(2);

        printf(" -> Posição 90 Graus (Pulso Médio ~300 ticks / 1.5ms)\n");
        set_all_pwm(fd, 0, 300);
        sleep(2);

        printf(" -> Posição 180 Graus (Pulso Longo ~500 ticks / 2.5ms)\n");
        set_all_pwm(fd, 0, 500);
        sleep(2);
    }

    close(fd);
    return 0;
}