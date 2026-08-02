#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

void write_reg(int fd, unsigned char reg, unsigned char val) {
    unsigned char buf[2] = {reg, val};
    if (write(fd, buf, 2) != 2) {
        perror("Erro na escrita I2C");
    }
}

unsigned char read_reg(int fd, unsigned char reg) {
    unsigned char val;
    write(fd, &reg, 1);
    read(fd, &val, 1);
    return val;
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
        perror("Falha ao abrir I2C");
        return 1;
    }

    // Configuração com depuração do registrador MODE1
    write_reg(fd, 0x00, 0x00); // Reset
    usleep(1000);

    // Coloca em SLEEP para ajustar Prescale
    write_reg(fd, 0x00, 0x10); 
    write_reg(fd, 0xFE, 121);  // 50Hz para servos
    write_reg(fd, 0x00, 0x20); // Sai do SLEEP e ativa Auto-incremento
    usleep(5000);

    write_reg(fd, 0x00, 0xa0); // Restart + Auto-incremento
    usleep(5000);

    unsigned char mode1 = read_reg(fd, 0x00);
    printf("[+] Registrador MODE1 lido: 0x%02X (Esperado: 0xA0 ou similar)\n", mode1);

    printf("[+] Testando canais 14 e 15 em pulso continuo...\n");

    // Varredura contínua para teste visual
    for (int i = 0; i < 5; i++) {
        printf(" -> Posicao A (100 ticks)\n");
        set_pwm(fd, 14, 0, 150);
        set_pwm(fd, 15, 0, 150);
        sleep(1);

        printf(" -> Posicao B (400 ticks)\n");
        set_pwm(fd, 14, 0, 450);
        set_pwm(fd, 15, 0, 450);
        sleep(1);
    }

    close(fd);
    return 0;
}