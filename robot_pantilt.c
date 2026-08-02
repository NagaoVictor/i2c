#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

#define CH_PAN  8
#define CH_TILT 9

// ==========================================
// AJUSTE OS LIMITES DE MOVIMENTO AQUI
// ==========================================
// PAN (Esquerda / Direita)
#define PAN_MIN_DEG   20.0f   // Aumente se bater na esquerda (ex: 30.0f)
#define PAN_MAX_DEG   160.0f  // Diminua se bater na direita (ex: 150.0f)

// TILT (Cima / Baixo)
#define TILT_MIN_DEG  70.0f   // Limite superior (olhar para cima)
#define TILT_MAX_DEG  95.0f   // Limite inferior (olhar para baixo - ajuste fino para não forçar)
// ==========================================

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

void set_servo_safe_angle(int fd, int channel, float angle) {
    float min_deg = PAN_MIN_DEG;
    float max_deg = PAN_MAX_DEG;

    if (channel == CH_TILT) {
        min_deg = TILT_MIN_DEG;
        max_deg = TILT_MAX_DEG;
    }

    if (angle < min_deg) angle = min_deg;
    if (angle > max_deg) angle = max_deg;

    int ticks = (int)(140.0f + (angle / 180.0f) * (480.0f - 140.0f));
    set_pwm(fd, channel, 0, ticks);
}

void pca9685_init(int fd) {
    write_reg(fd, 0x00, 0x00);
    usleep(1000);
    write_reg(fd, 0x00, 0x10);
    usleep(5000);
    write_reg(fd, 0xFE, 121);
    write_reg(fd, 0x00, 0xA0);
    usleep(10000);
}

int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0 || ioctl(fd, I2C_SLAVE, PCA9685_ADDR) < 0) {
        perror("[!] Erro I2C");
        return 1;
    }

    pca9685_init(fd);
    printf("[+] Testando limites de PAN (Esquerda/Direita) e TILT...\n");

    // Centro
    set_servo_safe_angle(fd, CH_PAN, 90.0f);
    set_servo_safe_angle(fd, CH_TILT, 90.0f);
    sleep(2);

    // Testa Limite Esquerdo
    printf(" -> Movendo para o limite MÍNIMO do Pan (PAN_MIN_DEG = %.1f)\n", PAN_MIN_DEG);
    set_servo_safe_angle(fd, CH_PAN, PAN_MIN_DEG);
    sleep(2);

    // Testa Limite Direito
    printf(" -> Movendo para o limite MÁXIMO do Pan (PAN_MAX_DEG = %.1f)\n", PAN_MAX_DEG);
    set_servo_safe_angle(fd, CH_PAN, PAN_MAX_DEG);
    sleep(2);

    // Retorna ao centro
    set_servo_safe_angle(fd, CH_PAN, 90.0f);
    sleep(1);

    close(fd);
    printf("[+] Calibração de eixos finalizada.\n");
    return 0;
}