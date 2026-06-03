#include "game.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(void) {
#ifdef _WIN32
    /* UTF-8 출력 설정 (한글/특수문자 깨짐 방지) */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
    run_game();
    return 0;
}
