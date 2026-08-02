#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

// Canais Confirmados no seu Hardware
#define CH_PAN  8
#define CH_TILT 9

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

// Converte Ângulo (0 a 180 graus) para Ticks de PWM do PCA9685
void set_angle(int fd, int ch, float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    
    // Mapeamento: 0 deg = 140 ticks | 180 deg = 480 ticks
    int ticks = (int)(140.0f + (angle / 180.0f) * (480.0f - 140.0f));
    set_pwm(fd, ch, 0, ticks);
}

int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0 || ioctl(fd, I2C_SLAVE, PCA9685_ADDR) < 0) {
        perror("[!] Erro I2C");
        return 1;
    }

    // Configuração do PCA9685 a 50Hz (Servos)
    write_reg(fd, 0x00, 0x00);
    usleep(1000);
    write_reg(fd, 0x00, 0x10); // SLEEP mode
    usleep(5000);
    write_reg(fd, 0xFE, 121);  // Prescale para 50Hz
    write_reg(fd, 0x00, 0xA0); // Auto-Increment + Restart
    usleep(10000);

    printf("[+] Centralizando Câmera (Pan = 90°, Tilt = 90°)...\n");
    set_angle(fd, CH_PAN, 90.0f);
    set_angle(fd, CH_TILT, 90.0f);
    sleep(2);

    printf("[+] Testando Movimento PAN (Canal 8 - Horizontal)...\n");
    set_angle(fd, CH_PAN, 30.0f);  // Olhar Esquerda
    sleep(1);
    set_angle(fd, CH_PAN, 150.0f); // Olhar Direita
    sleep(1);
    set_angle(fd, CH_PAN, 90.0f);  // Voltar ao Centro
    sleep(1);

    printf("[+] Testando Movimento TILT (Canal 9 - Vertical)...\n");
    set_angle(fd, CH_TILT, 45.0f);  // Olhar para Cima
    sleep(1);
    set_angle(fd, CH_TILT, 135.0f); // Olhar para Baixo
    sleep(1);
    set_angle(fd, CH_TILT, 90.0f);  // Voltar ao Centro
    sleep(1);

    printf("[+] Teste concluído com sucesso nos canais 8 e 9!\n");
    close(fd);
    return 0;
}