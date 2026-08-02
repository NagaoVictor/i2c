#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_BUS "/dev/i2c-1"
#define PCA9685_ADDR 0x40

#define CH_PAN  8
#define CH_TILT 9

// Limites seguros de hardware (evita forçar engrenagens)
#define PAN_MIN_DEG   10.0f
#define PAN_MAX_DEG   170.0f

#define TILT_MIN_DEG  40.0f   // Evita forçar para CIMA
#define TILT_MAX_DEG  125.0f  // Evita forçar para BAIXO (Ajustado!)

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

    // Aplica a trava de segurança no ângulo
    if (angle < min_deg) angle = min_deg;
    if (angle > max_deg) angle = max_deg;

    // Mapeia graus para ticks (140 a 480 ticks)
    int ticks = (int)(140.0f + (angle / 180.0f) * (480.0f - 140.0f));
    set_pwm(fd, channel, 0, ticks);
}