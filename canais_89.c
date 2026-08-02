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

    // Inicializa PCA9685
    write_reg(fd, 0x00, 0x00);
    usleep(1000);
    write_reg(fd, 0x00, 0x10);
    usleep(5000);
    write_reg(fd, 0xFE, 121);
    write_reg(fd, 0x00, 0xA0);
    usleep(10000);

    printf("[+] Testando Canal 8 isoladamente (Movendo para 300 ticks)...\n");
    set_pwm(fd, 8, 0, 300);
    sleep(2);
    set_pwm(fd, 8, 0, 400);
    sleep(2);

    printf("[+] Testando Canal 9 isoladamente (Movendo para 300 ticks)...\n");
    set_pwm(fd, 9, 0, 300);
    sleep(2);
    set_pwm(fd, 9, 0, 400);
    sleep(2);

    close(fd);
    printf("Teste de canais finalizado.\n");
    return 0;
}