#include "game.h"

/* ------------------------------------------------
   print_title
   ------------------------------------------------ */
void print_title() {
    printf("\n");
    printf("  ╔════════════════════════════════════════════╗\n");
    printf("  ║                                            ║\n");
    printf("  ║         수능 RPG : 공부는 전쟁이다         ║\n");
    printf("  ║                                            ║\n");
    printf("  ║    중학교 → 고등학교 → 수능을 정복하라!    ║\n");
    printf("  ║                                            ║\n");
    printf("  ╚════════════════════════════════════════════╝\n");
    printf("\n");
}

/* ------------------------------------------------
   clear_screen
   ------------------------------------------------ */
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* ------------------------------------------------
   print_separator
   ------------------------------------------------ */
void print_separator() {
    printf("  ════════════════════════════════════════════\n");
}

/* ------------------------------------------------
   print_status_flags
   Decodes bitmask and prints human-readable status
   ------------------------------------------------ */
void print_status_flags(unsigned int flags) {
    if (flags == STATUS_NORMAL) {
        printf("정상");
    } else {
        if (flags & STATUS_TIRED)     printf("[피곤]");
        if (flags & STATUS_SICK)      printf("[아픔]");
        if (flags & STATUS_FOCUSED)   printf("[집중]");
        if (flags & STATUS_STRESSED)  printf("[스트레스]");
        if (flags & STATUS_ENERGIZED) printf("[에너지]");
        if (flags & STATUS_EXAM_MODE) printf("[시험중]");
        if (flags & STATUS_GAME_OVER) printf("[게임오버]");
    }
}

/* ------------------------------------------------
   get_int_input
   Safe integer input with range checking
   ------------------------------------------------ */
int get_int_input(const char *prompt, int min, int max) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1) {
            /* Consume leftover newline */
            while (getchar() != '\n');
            if (value >= min && value <= max)
                return value;
        } else {
            /* Clear invalid input */
            while (getchar() != '\n');
        }
        printf("  %d에서 %d 사이의 숫자를 입력하세요.\n", min, max);
    }
}

/* ------------------------------------------------
   press_enter
   ------------------------------------------------ */
void press_enter() {
    printf("\n  [Enter를 눌러 계속...]");
    while (getchar() != '\n');
}
