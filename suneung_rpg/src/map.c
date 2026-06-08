#include "game.h"

/*
   MAP LAYOUT  (MAP_ROWS=8, MAP_COLS=10)
   Each cell is one character.

   Legend:
     #  = 벽/경계
     .  = 빈 땅 (이동 불가)
        = 도로 (이동 가능)
     H  = 집
     S  = 학교
     A  = 학원
     P  = PC방
     C  = 편의점
     L  = 도서관
     K  = 공원
     R  = 독서실
     @  = 플레이어 (런타임에 덮어씀)

   좌표: grid[row][col], row 0=위, row 7=아래
*/

static const char BASE_MAP[MAP_ROWS][MAP_COLS + 1] = {
    "##########",   /* row 0: 위쪽 벽 */
    "#S  .  C #",   /* row 1: 학교(1,1), 편의점(1,7) */
    "#   .    #",   /* row 2: 도로 */
    "#L  K  R #",   /* row 3: 도서관(3,1), 공원(3,4), 독서실(3,7) */
    "#   .    #",   /* row 4: 도로 */
    "#H  .  P #",   /* row 5: 집(5,1), PC방(5,7) */
    "#   .  A #",   /* row 6: 도로, 학원(6,7) */
    "##########"    /* row 7: 아래쪽 벽 */
};

/* Tile display strings with color codes (ANSI) */
/* Format: tile_char -> display label shown in legend */
typedef struct {
    char  tile;
    const char *label;
    const char *color; /* ANSI escape */
} TileInfo;

static const TileInfo TILE_TABLE[] = {
    { TILE_HOME,      "H:집      ", "\033[33m" },   /* yellow */
    { TILE_SCHOOL,    "S:학교    ", "\033[36m" },   /* cyan */
    { TILE_HAGWON,    "A:학원    ", "\033[35m" },   /* magenta */
    { TILE_PCBANG,    "P:PC방    ", "\033[31m" },   /* red */
    { TILE_STORE,     "C:편의점  ", "\033[32m" },   /* green */
    { TILE_LIBRARY,   "L:도서관  ", "\033[34m" },   /* blue */
    { TILE_PARK,      "K:공원    ", "\033[32m" },   /* green */
    { TILE_STUDYROOM, "R:독서실  ", "\033[35m" },   /* magenta */
    { TILE_PLAYER,    "@:나      ", "\033[1;37m"},   /* bold white */
    { 0, NULL, NULL }
};
#define ANSI_RESET "\033[0m"

/* ------------------------------------------------
   init_map
   Copies the base map template into the GameMap,
   places player at starting position (home).
   ------------------------------------------------ */
void init_map(GameMap *m) {
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            m->grid[r][c] = BASE_MAP[r][c];
        }
    }
    /* Player starts at home: row 5, col 1 */
    m->player_row  = 5;
    m->player_col  = 1;
    m->under_player = TILE_HOME;
    m->grid[m->player_row][m->player_col] = TILE_PLAYER;
}

/* ------------------------------------------------
   print_map
   Renders the 2D grid with a legend.
   ------------------------------------------------ */
void print_map(const GameMap *m) {
    printf("\n  +----- 동네 지도 ------+\n");

    for (int r = 0; r < MAP_ROWS; r++) {
        printf("  |");
        for (int c = 0; c < MAP_COLS; c++) {
            char tile = m->grid[r][c];
            /* Colorize special tiles */
            const char *color = NULL;
            for (int t = 0; TILE_TABLE[t].tile != 0; t++) {
                if (TILE_TABLE[t].tile == tile) {
                    color = TILE_TABLE[t].color;
                    break;
                }
            }
            if (color) printf("%s%c%s", color, tile, ANSI_RESET);
            else        printf("%c", tile);
        }
        printf("|\n");
    }

    printf("  +----------------------+\n");

    /* Legend */
    printf("  ");
    for (int t = 0; TILE_TABLE[t].tile != 0; t++) {
        printf("%s%s%s  ",
               TILE_TABLE[t].color,
               TILE_TABLE[t].label,
               ANSI_RESET);
        if ((t + 1) % 4 == 0) printf("\n  ");
    }
    printf("\n");
    printf("  이동: W(위) A(왼) S(아래) D(오른) | Q: 행동 메뉴\n");
}

/* ------------------------------------------------
   move_player
   Moves player one step in direction (W/A/S/D).
   Returns the tile the player lands on,
   or 0 if the move was blocked.
   ------------------------------------------------ */
int move_player(GameMap *m, char direction) {
    int new_row = m->player_row;
    int new_col = m->player_col;

    switch (direction) {
        case 'W': case 'w': new_row--; break;
        case 'S': case 's': new_row++; break;
        case 'A': case 'a': new_col--; break;
        case 'D': case 'd': new_col++; break;
        default: return 0;
    }

    /* Bounds check */
    if (new_row < 0 || new_row >= MAP_ROWS ||
        new_col < 0 || new_col >= MAP_COLS)
        return 0;

    char dest = m->grid[new_row][new_col];

    /* Can't walk into walls or impassable terrain */
    if (dest == TILE_WALL || dest == '#')
        return 0;

    /* Restore tile under old player position */
    m->grid[m->player_row][m->player_col] = m->under_player;

    /* Save tile at new position, place player */
    m->under_player = dest;
    m->player_row   = new_row;
    m->player_col   = new_col;
    m->grid[new_row][new_col] = TILE_PLAYER;

    return (unsigned char)m->under_player;
}

/* ------------------------------------------------
   friend_event_study
   공부 장소에서 친구를 만날 확률 (40%)
   발동 시 XP 보너스 + 메시지 출력
   Returns 1 if event fired, 0 if not
   ------------------------------------------------ */
static int friend_event_study(Player *p, const char *place) {
    if ((rand() % 10) >= 4) return 0;   /* 40% 확률 */

    /* 친구 이름 랜덤 */
    const char *friends[] = { "민준", "서연", "지호", "수아", "태양", "하은" };
    const char *name = friends[rand() % 6];

    /* 같이 공부 메시지 랜덤 */
    const char *msgs[] = {
        "같이 공부하자! 모르는 거 서로 물어보면서 했다.",
        "경쟁심이 불타올랐다. 지기 싫어서 더 집중했다.",
        "서로 문제 내주면서 공부했다. 생각보다 효율이 좋았다.",
        "공부하다 잠깐 수다 떨었지만, 그래도 집중이 잘 됐다."
    };
    const char *msg = msgs[rand() % 4];

    int xp_bonus = 20 + (rand() % 21);   /* +20~40 XP 보너스 */

    printf("\n  [친구 등장!] %s이(가) %s에 있었다!\n", name, place);
    printf("  \"%s\"\n", msg);
    printf("  모든 과목 XP +%d!\n", xp_bonus);

    for (int i = 0; i < NUM_SUBJECTS; i++) {
        p->subjects[i].xp += xp_bonus;
        /* 레벨업 체크 */
        while (p->subjects[i].xp >= p->subjects[i].xp_to_next
               && p->subjects[i].score < 100) {
            p->subjects[i].xp       -= p->subjects[i].xp_to_next;
            p->subjects[i].score    += 1;
            p->subjects[i].xp_to_next = 100 + (p->subjects[i].score - 40) * 5;
            printf("  ★ %s 점수 상승! -> %d점\n",
                   p->subjects[i].name, p->subjects[i].score);
        }
    }
    return 1;
}

/* ------------------------------------------------
   friend_event_play
   노는 장소에서 친구를 만날 확률 (50%)
   발동 시 스트레스 추가 회복 + 돈/체력 추가 소모
   Returns 1 if event fired, 0 if not
   ------------------------------------------------ */
static int friend_event_play(Player *p, const char *place) {
    if ((rand() % 10) >= 5) return 0;   /* 50% 확률 */

    const char *friends[] = { "민준", "서연", "지호", "수아", "태양", "하은" };
    const char *name = friends[rand() % 6];

    /* 노는 이벤트 종류 */
    int event = rand() % 4;
    printf("\n  [친구 등장!] %s이(가) %s에 있었다!\n", name, place);

    switch (event) {
        case 0:
            printf("  같이 더 신나게 놀았다! (돈 -5,000원, 스트레스 -15 추가)\n");
            if (p->money >= 5000) {
                p->money  -= 5000;
                p->stress -= 15;
            } else {
                printf("  돈이 없어서 친구가 사줬다... 미안하다.\n");
                p->stress -= 10;
            }
            break;
        case 1:
            printf("  수다 떨다 보니 시간 가는 줄 몰랐다. (체력 -10, 스트레스 -20 추가)\n");
            p->stamina -= 10;
            p->stress  -= 20;
            break;
        case 2:
            printf("  친구가 간식을 사줬다! 체력 +15\n");
            p->stamina += 15;
            p->stress  -= 10;
            break;
        case 3:
            printf("  친구랑 내기 게임을 했다... 졌다. (돈 -3,000원)\n");
            if (p->money >= 3000) p->money -= 3000;
            p->stress  -= 5;   /* 그래도 스트레스는 풀림 */
            break;
    }

    if (p->stress < 0)               p->stress  = 0;
    if (p->stamina < 0)              p->stamina = 0;
    if (p->stamina > p->max_stamina) p->stamina = p->max_stamina;
    if (p->stress < 80) REMOVE_STATUS(p, STATUS_SICK);
    if (p->stress < 40) REMOVE_STATUS(p, STATUS_STRESSED);
    return 1;
}

/* ------------------------------------------------
   friend_event_store
   편의점에서 친구를 마주칠 확률 (30%)
   친구한테 간식 사줌 → 돈 소모, 기분 업
   ------------------------------------------------ */
static int friend_event_store(Player *p) {
    if ((rand() % 10) >= 3) return 0;   /* 30% 확률 */

    const char *friends[] = { "민준", "서연", "지호", "수아", "태양", "하은" };
    const char *name = friends[rand() % 6];

    int cost = 2000 + (rand() % 3) * 1000;  /* 2,000~4,000원 */
    printf("\n  [친구 등장!] 편의점에서 %s을(를) 우연히 만났다!\n", name);

    if (p->money >= cost) {
        printf("  간식을 사줬다. (-%d원)  스트레스 -10, 기분 좋다!\n", cost);
        p->money  -= cost;
        p->stress -= 10;
        if (p->stress < 0) p->stress = 0;
        REMOVE_STATUS(p, STATUS_STRESSED);
    } else {
        printf("  %s이(가) 간식을 사줬다. 고마워!\n  스트레스 -5\n", name);
        p->stress -= 5;
        if (p->stress < 0) p->stress = 0;
    }
    return 1;
}

/* ------------------------------------------------
   tile_action
   Performs the action for the tile the player
   is currently standing on.
   Returns AP consumed (양수), 0 (AP 소모 없는 행동), -1 (취소/실패).
   ------------------------------------------------ */
static int tile_action(Player *p, char tile) {
    switch (tile) {
        case TILE_HOME:
            printf("\n[집] 집에서 쉽니다... (AP -1)\n");
            rest(p);
            return AP_REST;  /* rest()는 항상 성공 */

        case TILE_SCHOOL:
            printf("\n[학교] 자습실에서 공부합니다. (AP -2)\n");
            if (!self_study(p)) return -1;
            friend_event_study(p, "자습실");
            return AP_STUDY;

        case TILE_HAGWON:
            printf("\n[학원] 어떤 과목 학원에 갈까요? (AP -2)\n");
            for (int i = 0; i < NUM_SUBJECTS; i++)
                printf("  %d. %s\n", i + 1, p->subjects[i].name);
            {
                int s = get_int_input("선택: ", 1, NUM_SUBJECTS);
                if (!go_to_hagwon(p, s - 1)) return -1;
            }
            return AP_STUDY;

        case TILE_PCBANG:
            printf("\n[PC방] 스트레스를 풀어봅시다. (AP -1)\n");
            if (!visit_pc_bang(p)) return -1;
            friend_event_play(p, "PC방");
            return AP_PLAY;

        case TILE_STORE: {
            printf("\n[편의점] 잔액: %d원\n\n", p->money);
            printf("  1. 물건 구매      (AP -0)\n");
            printf("  2. 알바하기       (AP -2, +15,000원)\n");
            printf("  3. 그냥 지나치기  (AP 소모 없음)\n");
            int menu = get_int_input("선택: ", 1, 3);

            if (menu == 3) return -1;

            /* ── 알바 ── */
            if (menu == 2) {
                if (HAS_STATUS(p, STATUS_SICK)) {
                    printf("  몸이 아파서 알바를 할 수 없습니다.\n");
                    press_enter();
                    return -1;  /* AP 소모 없이 취소, 다른 행동 가능 */
                }
                printf("\n  [편의점 알바] 4시간 근무...\n");
                int wage         = 15000;
                int stamina_cost = 30;
                int stress_gain  = 15;

                if (HAS_STATUS(p, STATUS_TIRED)) {
                    stress_gain += 10;
                    printf("  피곤한 상태라 더 힘드네...\n");
                }

                p->money   += wage;
                p->stamina -= stamina_cost;
                p->stress  += stress_gain;
                if (p->stamina < 0)   p->stamina = 0;
                if (p->stress > 100)  p->stress  = 100;

                int event = rand() % 4;
                switch (event) {
                    case 0:
                        printf("  진상 손님을 만났다... 스트레스 +5\n");
                        p->stress += 5;
                        break;
                    case 1:
                        printf("  사장님한테 칭찬받았다! 용돈 +3,000원 팁!\n");
                        p->money += 3000;
                        break;
                    case 2:
                        printf("  조용한 알바였다. 틈틈이 공부도 했다.\n");
                        p->subjects[SUBJ_KOREAN].xp += 10;
                        break;
                    case 3:
                        printf("  유통기한 지난 삼각김밥을 얻었다. 체력 +5\n");
                        p->stamina += 5;
                        if (p->stamina > p->max_stamina) p->stamina = p->max_stamina;
                        break;
                }

                printf("  알바 완료! +%d원  (체력 -%d, 스트레스 +%d)\n",
                       wage, stamina_cost, stress_gain);

                if (p->stamina <= 30) ADD_STATUS(p, STATUS_TIRED);
                if (p->stress >= 80)  ADD_STATUS(p, STATUS_SICK);
                return AP_STORE_WORK;
            }

            /* ── 구매 (AP 소모 없음) ── */
            printf("\n  1. 에너지 드링크 (3,000원)  - 체력 +20\n");
            printf("  2. 비타민C       (5,000원)  - 스트레스 -20\n");
            printf("  3. 집중력 드링크 (8,000원)  - 집중 버프\n");
            printf("  4. 치킨          (15,000원) - 체력+30, 스트레스-15\n");
            printf("  5. 취소\n");
            int choice = get_int_input("선택: ", 1, 5);
            if (choice == 5) return -1;
            Item item;
            memset(&item, 0, sizeof(Item));
            item.quantity = 1;
            int cost = 0;
            switch (choice) {
                case 1: strcpy(item.name,"에너지 드링크"); strcpy(item.description,"체력 +20");             item.stamina_restore=20;                  cost=3000;  break;
                case 2: strcpy(item.name,"비타민C");       strcpy(item.description,"스트레스 -20");         item.stress_reduce=20;                    cost=5000;  break;
                case 3: strcpy(item.name,"집중력 드링크"); strcpy(item.description,"집중 버프");            item.study_bonus=1;                        cost=8000;  break;
                case 4: strcpy(item.name,"치킨");          strcpy(item.description,"체력+30, 스트레스-15"); item.stamina_restore=30; item.stress_reduce=15; cost=15000; break;
            }
            if (p->money < cost) { printf("돈이 부족합니다!\n"); return -1; }
            p->money -= cost;
            add_item(p, item);
            printf("[%s] 구매! (-%d원)\n", item.name, cost);
            friend_event_store(p);
            return AP_STORE_BUY;
        }

        case TILE_LIBRARY:
            printf("\n[도서관] 조용한 환경에서 집중 공부합니다. (AP -2)\n");
            ADD_STATUS(p, STATUS_FOCUSED);
            printf("  집중력 버프 발동!\n");
            if (!self_study(p)) {
                REMOVE_STATUS(p, STATUS_FOCUSED);  /* 공부 못 하면 버프도 취소 */
                return -1;
            }
            friend_event_study(p, "도서관");
            return AP_STUDY;

        case TILE_PARK:
            printf("\n[공원] 산책하며 스트레스를 해소합니다. (AP -1)\n");
            {
                int s_before = p->stress;
                p->stress -= 25;
                if (p->stress < 0) p->stress = 0;
                p->stamina += 15;
                if (p->stamina > p->max_stamina) p->stamina = p->max_stamina;
                printf("  스트레스 -%d, 체력 +15\n", s_before - p->stress);
                if (p->stress < 80) REMOVE_STATUS(p, STATUS_SICK);
                if (p->stress < 40) REMOVE_STATUS(p, STATUS_STRESSED);
                friend_event_play(p, "공원");
            }
            return AP_REST;

        case TILE_STUDYROOM:
            printf("\n[독서실] 조용한 독서실에서 밤새 공부합니다. (AP -2)\n");
            {
                int cost = 5000;
                if (p->money < cost) {
                    printf("  독서실 이용료 부족 (5,000원 필요)\n");
                    return -1;
                }
                p->money -= cost;
                ADD_STATUS(p, STATUS_FOCUSED);
                ADD_STATUS(p, STATUS_ENERGIZED);
                printf("  집중+에너지 버프 발동! (-%d원)\n", cost);
                if (!self_study(p)) {
                    /* 돈은 냈지만 공부 못한 경우 — AP는 소모 안 함 */
                    REMOVE_STATUS(p, STATUS_FOCUSED);
                    REMOVE_STATUS(p, STATUS_ENERGIZED);
                    return -1;
                }
                friend_event_study(p, "독서실");
            }
            return AP_STUDY;

        default:
            return -1;
    }
}

/* ------------------------------------------------
   run_map_day
   One full in-game day using the map interface.
   Player has AP_PER_DAY action points each day.
   Moving costs no AP; only tile actions consume AP.
   Day ends when AP reaches 0 or player presses Q.
   ------------------------------------------------ */
void run_map_day(Player *p, GameMap *m, int day) {
    /* daily_ap는 전날 apply_day_end()에서 이미 AP_PER_DAY로 초기화됨
       첫 날(init_player)도 AP_PER_DAY로 시작하므로 여기서는 초기화 불필요 */

    /* AP 바 출력 헬퍼 (람다 대신 매크로) */
    #define PRINT_AP_BAR(ap) \
        do { \
            printf("  AP: ["); \
            for (int _i = 0; _i < AP_PER_DAY; _i++) \
                printf(_i < (ap) ? "■" : "□"); \
            printf("] %d / %d\n", (ap), AP_PER_DAY); \
        } while(0)

    while (p->daily_ap > 0) {
        clear_screen();

        /* ── 헤더 ── */
        const char *year_names[] = {"", "고1", "고2", "고3"};
        const char *month_names[] = {
            "","1월","2월","3월","4월","5월","6월",
            "7월","8월","9월","10월","11월","12월"
        };
        int sem = (p->month <= 7) ? 1 : 2;
        printf("\n");
        printf("  ┌─────────────────────────────────────────┐\n");
        printf("  │  %s  %s  %d학기  %d일차  |  용돈: %d원\n",
               (p->year >= 1 && p->year <= 3) ? year_names[p->year] : "??",
               (p->month >= 1 && p->month <= 12) ? month_names[p->month] : "??월",
               sem, day, p->money);
        printf("  │  HP:%d  체력:%d  스트레스:%d  상태:",
               p->hp, p->stamina, p->stress);
        print_status_flags(p->status_flags);
        PRINT_AP_BAR(p->daily_ap);
        printf("  └─────────────────────────────────────────┘\n");

        /* AP 소모 안내 */
        printf("  [AP]  자습·학원·도서관·독서실·알바: 2  |  집·공원·PC방: 1  |  편의점 구매·인벤·저장: 0\n");

        /* ── 지도 ── */
        print_map(m);

        /* 현재 위치 표시 */
        char cur = m->under_player;
        if (cur != TILE_ROAD && cur != TILE_EMPTY && cur != ' ') {
            printf("\n  >> 현재 위치: ");
            for (int t = 0; TILE_TABLE[t].tile != 0; t++) {
                if (TILE_TABLE[t].tile == cur) {
                    printf("%s%s%s", TILE_TABLE[t].color,
                           TILE_TABLE[t].label, ANSI_RESET);
                    break;
                }
            }
            printf(" << [Enter: 입장]  또는 이동\n");
        }

        printf("\n  입력 (W/A/S/D 이동 | Enter: 장소 입장 | I: 인벤 | T: 성적 | V: 저장 | Q: 하루 끝내기): ");
        fflush(stdout);

        /* 키 입력 */
        char input = 0;
        {
            char buf[8];
            if (fgets(buf, sizeof(buf), stdin))
                input = buf[0];
        }

        if (input == '\n' || input == '\r' || input == 0) {
            /* Enter → 장소 입장 */
            int ap_cost = tile_action(p, m->under_player);
            if (ap_cost > 0) {
                /* AP 충분한지 검사 */
                if (ap_cost > p->daily_ap) {
                    printf("\n  AP가 부족합니다! (필요: %d, 남은 AP: %d)\n",
                           ap_cost, p->daily_ap);
                    printf("  남은 AP로 할 수 있는 행동을 선택하거나 Q로 하루를 마무리하세요.\n");
                    press_enter();
                } else {
                    p->daily_ap -= ap_cost;
                    press_enter();
                    if (p->daily_ap == 0) {
                        printf("\n  ── AP를 모두 소모했습니다. 오늘 하루 수고했어요! ──\n");
                        press_enter();
                    }
                }
            } else if (ap_cost == 0) {
                /* AP_STORE_BUY 등 AP 소모 없는 행동 — press_enter만 */
                press_enter();
            }
            /* ap_cost < 0 (행동 취소/무효) 는 아무것도 하지 않음 */
        }
        else if (input == 'W' || input == 'w' ||
                 input == 'A' || input == 'a' ||
                 input == 'S' || input == 's' ||
                 input == 'D' || input == 'd') {
            move_player(m, input);  /* 이동은 AP 소모 없음 */
        }
        else if (input == 'I' || input == 'i') {
            if (p->item_count == 0) {
                printf("  [인벤토리가 비어 있습니다.]\n");
            } else {
                printf("\n[인벤토리]\n");
                for (int i = 0; i < p->item_count; i++)
                    printf("  %d. %-15s x%d - %s\n",
                           i + 1, p->inventory[i].name,
                           p->inventory[i].quantity,
                           p->inventory[i].description);
                printf("  %d. 닫기\n", p->item_count + 1);
                int c = get_int_input("사용할 번호: ", 1, p->item_count + 1);
                if (c <= p->item_count) use_item(p, c - 1);
            }
            press_enter();
        }
        else if (input == 'T' || input == 't') {
            print_subjects(p);
            press_enter();
        }
        else if (input == 'V' || input == 'v') {
            save_game(p);
            press_enter();
        }
        else if (input == 'Q' || input == 'q') {
            if (p->daily_ap == AP_PER_DAY) {
                /* 아무것도 안 한 경우 */
                printf("  [하루를 아무것도 안 하고 보냈다...] 스트레스 +3\n");
                p->stress += 3;
            } else {
                printf("  [남은 AP %d를 흘려보냈다. 조금 더 할 수 있었는데...]\n",
                       p->daily_ap);
            }
            press_enter();
            break;  /* AP 남아도 하루 종료 */
        }
    }

    #undef PRINT_AP_BAR

    apply_day_end(p);
}
