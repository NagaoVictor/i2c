#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

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

// Função de Tilt à prova de falhas baseada estritamente em Ticks
void set_tilt_safe_ticks(int fd, int ticks) {
    // -----------------------------------------------------------------
    // DEFINA O TETO E O PISO DE TICKS PARA O SEU TILT AQUI:
    // Se o limite inferior (onde ele bate/desce demais) for em X ticks,
    // nós travamos o valor mínimo aqui.
    // -----------------------------------------------------------------
    int TICKS_MINIMO_PERMITIDO = 300; // Ajuste se precisar subir/descer mais
    int TICKS_MAXIMO_PERMITIDO = 420; // Ajuste o limite do outro lado

    if (ticks < TICKS_MINIMO_PERMITIDO) {
        ticks = TICKS_MINIMO_PERMITIDO;
    }
    if (ticks > TICKS_MAXIMO_PERMITIDO) {
        ticks = TICKS_MAXIMO_PERMITIDO;
    }

    set_pwm(fd, CH_TILT, 0, ticks);
}

void set_pan_ticks(int fd, int ticks) {
    if (ticks < 150) ticks = 150;
    if (ticks > 450) ticks = 450;
    set_pwm(fd, CH_PAN, 0, ticks);
}

void pca9685_init(int fd) {
    write_reg(fd, 0x00, 0x00);
    usleep(1000);
    write_reg(fd, 0x00, 0x10);
    usleep(5000);
    write_reg(fd, 0xFE, 121);  // 50Hz
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
    printf("[+] Testando controle restrito de Ticks no Tilt...\n");

    // Centraliza
    set_pan_ticks(fd, 312);
    set_tilt_safe_ticks(fd, 312);
    sleep(2);

    // Tenta mandar para um valor baixo (que faria descer) -> Deve travar em 300
    printf(" -> Tentando mandar ticks baixos (deve ser bloqueado pela trava)\n");
    set_tilt_safe_ticks(fd, 150); 
    sleep(2);

    // Retorna ao centro
    set_tilt_safe_ticks(fd, 312);

    close(fd);
    printf("[+] Concluído.\n");
    return 0;
}