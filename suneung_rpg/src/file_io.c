#include "game.h"

/* ------------------------------------------------
   save_game
   Writes Player struct to binary file
   Returns 1 on success, 0 on failure
   ------------------------------------------------ */
int save_game(const Player *p) {
    FILE *fp = fopen(SAVE_FILE, "wb");
    if (!fp) {
        printf("[저장 오류] 파일을 열 수 없습니다: %s\n", SAVE_FILE);
        return 0;
    }

    /* Write a magic number to verify file integrity */
    /* 0x53554E45 = "SUNE" in ASCII */
    unsigned int magic = 0x53554E45;
    fwrite(&magic, sizeof(unsigned int), 1, fp);

    /* Write the entire Player struct */
    fwrite(p, sizeof(Player), 1, fp);

    fclose(fp);
    printf("[저장 완료] 게임이 저장되었습니다.\n");
    return 1;
}

/* ------------------------------------------------
   save_exists / autosave_exists
   파일 존재 여부 확인
   ------------------------------------------------ */
int save_exists() {
    FILE *fp = fopen(SAVE_FILE, "rb");
    if (!fp) return 0;
    fclose(fp);
    return 1;
}

int autosave_exists() {
    FILE *fp = fopen(AUTOSAVE_FILE, "rb");
    if (!fp) return 0;
    fclose(fp);
    return 1;
}

/* ------------------------------------------------
   autosave
   자동저장 — AUTOSAVE_FILE에 저장, 메시지 간략히
   ------------------------------------------------ */
int autosave(const Player *p) {
    FILE *fp = fopen(AUTOSAVE_FILE, "wb");
    if (!fp) return 0;

    unsigned int magic = 0x53554E45;
    fwrite(&magic, sizeof(unsigned int), 1, fp);
    fwrite(p, sizeof(Player), 1, fp);
    fclose(fp);

    printf("  [자동저장 완료]\n");
    return 1;
}

/* ------------------------------------------------
   load_autosave
   자동저장 파일에서 불러오기
   ------------------------------------------------ */
int load_autosave(Player *p) {
    FILE *fp = fopen(AUTOSAVE_FILE, "rb");
    if (!fp) {
        printf("[자동저장 없음] 자동저장 파일이 없습니다.\n");
        return 0;
    }

    unsigned int magic = 0;
    fread(&magic, sizeof(unsigned int), 1, fp);
    if (magic != 0x53554E45) {
        printf("[자동저장 오류] 파일이 손상되었습니다.\n");
        fclose(fp);
        return 0;
    }

    size_t read = fread(p, sizeof(Player), 1, fp);
    fclose(fp);

    if (read != 1) {
        printf("[자동저장 오류] 데이터를 읽을 수 없습니다.\n");
        return 0;
    }

    printf("[자동저장 불러오기 완료] %s의 게임을 불러왔습니다.\n", p->name);
    return 1;
}
int load_game(Player *p) {
    FILE *fp = fopen(SAVE_FILE, "rb");
    if (!fp) {
        printf("[불러오기] 저장 파일이 없습니다. 새 게임을 시작하세요.\n");
        return 0;
    }

    /* Verify magic number */
    unsigned int magic = 0;
    fread(&magic, sizeof(unsigned int), 1, fp);
    if (magic != 0x53554E45) {
        printf("[불러오기 오류] 파일이 손상되었습니다.\n");
        fclose(fp);
        return 0;
    }

    /* Read Player struct */
    size_t read = fread(p, sizeof(Player), 1, fp);
    fclose(fp);

    if (read != 1) {
        printf("[불러오기 오류] 데이터를 읽을 수 없습니다.\n");
        return 0;
    }

    printf("[불러오기 완료] %s의 게임을 불러왔습니다.\n", p->name);
    return 1;
}
