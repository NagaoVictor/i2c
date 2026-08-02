#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

// Canais confirmados na sua bancada
#define CH_PAN  8
#define CH_TILT 9

// Limites seguros de hardware (evita forçar engrenagens e o tranco no batente)
#define PAN_MIN_DEG   10.0f
#define PAN_MAX_DEG   170.0f

#define TILT_MIN_DEG  40.0f   // Limite para CIMA
#define TILT_MAX_DEG  125.0f  // Limite para BAIXO (Ajustado para não forçar!)

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

// Envia o ângulo garantindo que o servo nunca passe dos limites físicos do suporte
void set_servo_safe_angle(int fd, int channel, float angle) {
    float min_deg = PAN_MIN_DEG;
    float max_deg = PAN_MAX_DEG;

    if (channel == CH_TILT) {
        min_deg = TILT_MIN_DEG;
        max_deg = TILT_MAX_DEG;
    }

    // Trava de segurança por software
    if (angle < min_deg) angle = min_deg;
    if (angle > max_deg) angle = max_deg;

    // Mapeamento: 0° = 140 ticks | 180° = 480 ticks
    int ticks = (int)(140.0f + (angle / 180.0f) * (480.0f - 140.0f));
    set_pwm(fd, channel, 0, ticks);
}

void pca9685_init(int fd) {
    write_reg(fd, 0x00, 0x00); // Reset
    usleep(1000);
    write_reg(fd, 0x00, 0x10); // SLEEP mode para alterar PRESCALE
    usleep(5000);
    write_reg(fd, 0xFE, 121);  // Prescale para 50Hz (Servos)
    write_reg(fd, 0x00, 0xA0); // Auto-Incremento + Restart
    usleep(10000);
}

// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------
int main() {
    int fd = open(I2C_BUS, O_RDWR);
    if (fd < 0 || ioctl(fd, I2C_SLAVE, PCA9685_ADDR) < 0) {
        perror("[!] Erro ao abrir barramento I2C");
        return 1;
    }

    // 1. Inicializa o gerador PWM no PCA9685
    pca9685_init(fd);
    printf("[+] PCA9685 Inicializado a 50Hz.\n");

    // 2. Centraliza Pan e Tilt suavemente
    printf("[+] Centralizando Pan (90°) e Tilt (90°)...\n");
    set_servo_safe_angle(fd, CH_PAN, 90.0f);
    set_servo_safe_angle(fd, CH_TILT, 90.0f);
    sleep(2);

    // 3. Teste de Varredura Suave de Visão
    printf("[+] Testando Varredura Horizontal (PAN)...\n");
    set_servo_safe_angle(fd, CH_PAN, 30.0f);   // Olhar Esquerda
    sleep(1);
    set_servo_safe_angle(fd, CH_PAN, 150.0f);  // Olhar Direita
    sleep(1);
    set_servo_safe_angle(fd, CH_PAN, 90.0f);   // Centro
    sleep(1);

    printf("[+] Testando Varredura Vertical com Trava (TILT)...\n");
    set_servo_safe_angle(fd, CH_TILT, 40.0f);  // Olhar para Cima (Limite Seguro)
    sleep(1);
    set_servo_safe_angle(fd, CH_TILT, 125.0f); // Olhar para Baixo (Sem bater no batente!)
    sleep(1);

    // 4. Retorna ao estado neutro de repouso
    printf("[+] Posição neutra ajustada. Finalizado sem ruídos ou trancos.\n");
    set_servo_safe_angle(fd, CH_PAN, 90.0f);
    set_servo_safe_angle(fd, CH_TILT, 90.0f);

    close(fd);
    return 0;
}