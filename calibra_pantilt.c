#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

// Canais do Pan e Tilt
#define CH_PAN  14
#define CH_TILT 15

// Valores de centro seguros para inicialização
#define CENTRO_TICKS 312 

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

    // Inicialização do PCA9685
    write_reg(fd, 0x00, 0x00);
    usleep(1000);
    write_reg(fd, 0x00, 0x10); // SLEEP
    usleep(5000);
    write_reg(fd, 0xFE, 121);  // 50Hz
    write_reg(fd, 0x00, 0xA0); // Restart + Auto-Increment
    usleep(10000);

    int val_pan = CENTRO_TICKS;
    int val_tilt = CENTRO_TICKS;

    // Posiciona em 90 graus imediatamente
    set_pwm(fd, CH_PAN, 0, val_pan);
    set_pwm(fd, CH_TILT, 0, val_tilt);

    printf("==================================================\n");
    printf("   CALIBRAÇÃO DE PAN/TILT (Canais %d e %d)\n", CH_PAN, CH_TILT);
    printf("==================================================\n");
    printf("Comandos:\n");
    printf("  a / d : Mover PAN  (Esquerda / Direita) [-10 / +10 ticks]\n");
    printf("  w / s : Mover TILT (Cima / Baixo)       [+10 / -10 ticks]\n");
    printf("  c     : Centralizar Ambos (312 ticks)\n");
    printf("  q     : Sair\n");
    printf("--------------------------------------------------\n");

    char op;
    while (1) {
        printf("\r[PAN: %d ticks] | [TILT: %d ticks] -> Opção: ", val_pan, val_tilt);
        fflush(stdout);
        scanf(" %c", &op);

        if (op == 'q') break;

        switch (op) {
            case 'a': val_pan -= 15; break;
            case 'd': val_pan += 15; break;
            case 'w': val_tilt += 15; break;
            case 's': val_tilt -= 15; break;
            case 'c': val_pan = CENTRO_TICKS; val_tilt = CENTRO_TICKS; break;
            default: continue;
        }

        // Limite de segurança geral para evitar quebrar engrenagens
        if (val_pan < 140) val_pan = 140;
        if (val_pan > 480) val_pan = 480;
        if (val_tilt < 140) val_tilt = 140;
        if (val_tilt > 480) val_tilt = 480;

        set_pwm(fd, CH_PAN, 0, val_pan);
        set_pwm(fd, CH_TILT, 0, val_tilt);
    }

    close(fd);
    return 0;
}