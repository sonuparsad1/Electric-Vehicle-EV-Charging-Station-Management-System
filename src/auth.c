#include "auth.h"
#include "common.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned long long fnv1a_hash(const char *s) {
    unsigned long long h = 1469598103934665603ULL;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 1099511628211ULL;
    }
    return h;
}

static void loading_animation(const char *msg, int steps, int delay_us) {
    printf("%s", msg);
    fflush(stdout);
    for (int i = 0; i < steps; i++) {
        printf(".");
        fflush(stdout);
        sleep_ms(delay_us / 1000);
    }
    printf("\n");
}

static void ensure_default_admin(void) {
    FILE *fp = fopen(AUTH_FILE, "r");
    if (fp) {
        fclose(fp);
        return;
    }

    fp = fopen(AUTH_FILE, "w");
    if (!fp) return;

    const char *default_user = "admin";
    const char *default_pass = "admin123";
    unsigned long long hash = fnv1a_hash(default_pass);
    fprintf(fp, "%s:%llu\n", default_user, hash);
    fclose(fp);
}

static int validate_credentials(const char *user, const char *pass) {
    FILE *fp = fopen(AUTH_FILE, "r");
    if (!fp) return 0;

    char line[256];
    unsigned long long input_hash = fnv1a_hash(pass);
    int ok = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *sep = strchr(line, ':');
        if (!sep) continue;
        *sep = '\0';
        char *hash_str = sep + 1;
        hash_str[strcspn(hash_str, "\r\n")] = '\0';

        if (strcmp(line, user) == 0) {
            unsigned long long stored = strtoull(hash_str, NULL, 10);
            if (stored == input_hash) ok = 1;
            break;
        }
    }

    fclose(fp);
    return ok;
}

int auth_login_screen(void) {
    ensure_default_admin();
    int attempts = 0;

    while (attempts < 3) {
        clear_screen();
        printf(COLOR_CYAN COLOR_BOLD "===============================================\n" COLOR_RESET);
        printf(COLOR_CYAN COLOR_BOLD "      EV CHARGING STATION - ADMIN LOGIN\n" COLOR_RESET);
        printf(COLOR_CYAN COLOR_BOLD "===============================================\n" COLOR_RESET);

        char username[64], password[64];
        printf("Username: ");
        if (!fgets(username, sizeof(username), stdin)) return 0;
        username[strcspn(username, "\r\n")] = '\0';

        printf("Password: ");
        if (!fgets(password, sizeof(password), stdin)) return 0;
        password[strcspn(password, "\r\n")] = '\0';

        loading_animation("Authenticating", 3, 140000);

        int ok = validate_credentials(username, password);
        log_login_attempt(username, ok);

        if (ok) {
            printf(COLOR_GREEN "Login successful.\n" COLOR_RESET);
            sleep_ms(300);
            return 1;
        }

        attempts++;
        printf(COLOR_RED "Invalid credentials. Attempts left: %d\n" COLOR_RESET, 3 - attempts);
        sleep_ms(800);
    }

    log_event("AUTH", "System locked after 3 failed login attempts");
    clear_screen();
    printf(COLOR_RED COLOR_BOLD "SYSTEM LOCKED: Too many failed attempts.\n" COLOR_RESET);
    return 0;
}
