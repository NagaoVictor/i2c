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
    ioctl(fd, I2C_SLAVE, PCA9685_ADDR);

    // Configura frequência para 50Hz (Servos)
    write_reg(fd, 0x00, 0x10); // Sleep
    write_reg(fd, 0xFE, 121);  // Prescale 50Hz
    write_reg(fd, 0x00, 0x20); // Auto-increment
    usleep(5000);

    printf("[+] Movendo Servo Pan (Canal 14) e Tilt (Canal 15)...\n");

    // Centro (90 graus ~ 307 ticks)
    set_pwm(fd, 14, 0, 307);
    set_pwm(fd, 15, 0, 307);
    sleep(1);

    // Move Pan para Esquerda (0 deg ~ 130 ticks)
    set_pwm(fd, 14, 0, 130);
    sleep(1);

    // Move Pan para Direita (180 deg ~ 480 ticks)
    set_pwm(fd, 14, 0, 480);
    sleep(1);

    // Volta ao centro
    set_pwm(fd, 14, 0, 307);

    close(fd);
    return 0;
}