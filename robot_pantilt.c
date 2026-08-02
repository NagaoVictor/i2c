#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

// Canais corrigidos e confirmados na sua bancada
#define CH_TILT 8   // Cima / Baixo
#define CH_PAN  9   // Esquerda / Direita

// Limites seguros baseados na sua calibração
#define TILT_MIN_TICKS  240   // Limite inferior seguro (não desce mais que isso)
#define TILT_MAX_TICKS  420   // Limite superior (olhar para cima)

#define PAN_MIN_TICKS   150   // Limite esquerdo
#define PAN_MAX_TICKS   450   // Limite direito

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

// Função segura para o TILT (Canal 8) - Nunca desce além de 240 ticks
void set_tilt_safe(int fd, int ticks) {
    if (ticks < TILT_MIN_TICKS) ticks = TILT_MIN_TICKS;
    if (ticks > TILT_MAX_TICKS) ticks = TILT_MAX_TICKS;
    set_pwm(fd, CH_TILT, 0, ticks);
}

// Função segura para o PAN (Canal 9)
void set_pan_safe(int fd, int ticks) {
    if (ticks < PAN_MIN_TICKS) ticks = PAN_MIN_TICKS;
    if (ticks > PAN_MAX_TICKS) ticks = PAN_MAX_TICKS;
    set_pwm(fd, CH_PAN, 0, ticks);
}

void pca9685_init(int fd) {
    write_reg(fd, 0x00, 0x00); // Reset
    usleep(1000);
    write_reg(fd, 0x00, 0x10); // SLEEP
    usleep(5000);
    write_reg(fd, 0xFE, 121);  // 50Hz
    write_reg(fd, 0x00, 0xA0); // Auto-Increment
    usleep(10000);
}

int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0 || ioctl(fd, I2C_SLAVE, PCA9685_ADDR) < 0) {
        perror("[!] Erro I2C");
        return 1;
    }

    pca9685_init(fd);
    printf("[+] PCA9685 Inicializado. Executando rotina segura...\n");

    // 1. Centraliza tudo (Pan no meio, Tilt no limite seguro inferior 240)
    printf(" -> Movendo para a posição inicial (Centro / Mínimo seguro)\n");
    set_pan_safe(fd, 300);
    set_tilt_safe(fd, TILT_MIN_TICKS); // 240 ticks
    sleep(2);

    // 2. Testa Pan (Esquerda e Direita)
    printf(" -> Girando para a Esquerda\n");
    set_pan_safe(fd, PAN_MIN_TICKS);
    sleep(1);

    printf(" -> Girando para a Direita\n");
    set_pan_safe(fd, PAN_MAX_TICKS);
    sleep(1);

    // Retorna Pan ao centro
    set_pan_safe(fd, 300);
    sleep(1);

    // 3. Testa Tilt para Cima (Subindo a cabeça)
    printf(" -> Levantando a cabeça (Tilt para cima)\n");
    set_tilt_safe(fd, 360);
    sleep(1);

    // Retorna Tilt ao ponto mínimo seguro (240) sem forçar
    printf(" -> Retornando ao ponto mínimo seguro (240 ticks)\n");
    set_tilt_safe(fd, TILT_MIN_TICKS);
    sleep(1);

    close(fd);
    printf("[+] Rotina finalizada com sucesso! Sem trancos.\n");
    return 0;
}