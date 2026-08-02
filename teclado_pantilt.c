#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

#define CH_TILT 8   // Cima / Baixo
#define CH_PAN  9   // Esquerda / Direita

#define TILT_MIN_TICKS  245   // Limite inferior seguro (não desce mais que isso)
#define TILT_MAX_TICKS  420   // Limite superior (olhar para cima)

#define PAN_MIN_TICKS   150   // Limite esquerdo
#define PAN_MAX_TICKS   450   // Limite direito

// Variáveis globais para rastrear a posição atual dos servos
int current_pan  = 300;
int current_tilt = 245;

// Configuração do terminal para leitura imediata (modo não-canônico)
struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // Desativa eco visual e espera por Enter
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

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

void update_servos(int fd) {
    // Trava de segurança no Tilt
    if (current_tilt < TILT_MIN_TICKS) current_tilt = TILT_MIN_TICKS;
    if (current_tilt > TILT_MAX_TICKS) current_tilt = TILT_MAX_TICKS;

    // Trava de segurança no Pan
    if (current_pan < PAN_MIN_TICKS) current_pan = PAN_MIN_TICKS;
    if (current_pan > PAN_MAX_TICKS) current_pan = PAN_MAX_TICKS;

    set_pwm(fd, CH_TILT, 0, current_tilt);
    set_pwm(fd, CH_PAN, 0, current_pan);
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
    
    // Posiciona no centro/mínimo inicial seguro
    update_servos(fd);

    enable_raw_mode();

    printf("==================================================\n");
    printf("     CONTROLE DO PAN/TILT VIA TECLADO (TERMIOS)   \n");
    printf("==================================================\n");
    printf(" [W] Cima        | [S] Baixo (Min seguro: 245)   \n");
    printf(" [A] Esquerda    | [D] Direita                   \n");
    printf(" [Q] Sair                                        \n");
    printf("--------------------------------------------------\n");

    char c;
    int step = 10; // Tamanho do passo de movimento por toque na tecla

    while (1) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            // Aceita letras maiúsculas e minúsculas
            if (c == 'q' || c == 'Q') {
                break;
            } else if (c == 'w' || c == 'W') {
                current_tilt += step; // Sobe
            } else if (c == 's' || c == 'S') {
                current_tilt -= step; // Desce (travado em 245)
            } else if (c == 'a' || c == 'A') {
                current_pan -= step;  // Esquerda
            } else if (c == 'd' || c == 'D') {
                current_pan += step;  // Direita
            }

            update_servos(fd);
            
            // Exibe o status atual no terminal em tempo real
            printf("\r[Status] Pan: %-4d | Tilt: %-4d   ", current_pan, current_tilt);
            fflush(stdout);
        }
    }

    // Centraliza ao sair por segurança
    current_pan = 300;
    current_tilt = 245;
    update_servos(fd);

    close(fd);
    printf("\n[+] Programa encerrado.\n");
    return 0;
}