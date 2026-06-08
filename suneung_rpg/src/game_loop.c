#include "game.h"

/*
   월별 이벤트 구조 (고1~고3 공통):

   1학기
     3월: 자유 학습 + 3월 모의고사
     4월: 자유 학습
     5월: 자유 학습 + 1학기 중간고사
     6월: 자유 학습 + 6월 모의고사
     7월: 자유 학습 + 1학기 기말고사

   (8월: 여름방학 — 스킵, 체력/스트레스 회복)

   2학기
     9월: 자유 학습 + 9월 모의고사
    10월: 자유 학습
    11월: 자유 학습 + 2학기 중간고사
    12월: 자유 학습 + 2학기 기말고사
         (고3 2학기는 기말 대신 수능)

   다음 학년으로 넘어가면 year++, month=3, age++
*/

#define DAYS_PER_MONTH   10   /* 압축: 한 달 = 10일 */

/* 모의고사가 있는 월 */
#define MOCK_MONTH_3    3
#define MOCK_MONTH_6    6
#define MOCK_MONTH_9    9

/* 시험이 있는 월 */
#define MIDTERM_1_MONTH  5   /* 1학기 중간 */
#define FINAL_1_MONTH    7   /* 1학기 기말 */
#define MIDTERM_2_MONTH 11   /* 2학기 중간 */
#define FINAL_2_MONTH   12   /* 2학기 기말 / 수능 */

/* ------------------------------------------------
   get_semester
   ------------------------------------------------ */
int get_semester(int month) {
    return (month <= 7) ? 1 : 2;
}

/* ------------------------------------------------
   run_exam
   ------------------------------------------------ */
static void run_exam(Player *p, int exam_type) {
    /* exam_type: 0=모의, 1=중간, 2=기말, 3=수능 */
    clear_screen();
    printf("\n");
    print_separator();
    if (exam_type == 3)
        printf("  !! 드디어 수능 날입니다 !!\n");
    else if (exam_type == 0)
        printf("  !! %d월 모의고사입니다 !!\n", p->month);
    else
        printf("  !! %s입니다 !!\n", exam_type == 1 ? "중간고사" : "기말고사");
    print_separator();

    Boss boss = create_boss(p->year, p->month, exam_type);
    int result = run_battle(p, &boss);

    p->total_score = get_total_score(p);

    if (!result) {
        for (int i = 0; i < NUM_SUBJECTS; i++) {
            p->subjects[i].score -= 2;
            if (p->subjects[i].score < 0) p->subjects[i].score = 0;
        }
        printf("  ※ 모든 과목 점수 -2점 (좌절 패널티)\n");
    }

    autosave(p);   /* 시험 후 자동저장 */
    press_enter();
}

/* ------------------------------------------------
   summer_vacation
   8월 여름방학 처리
   ------------------------------------------------ */
static void summer_vacation(Player *p) {
    clear_screen();
    printf("\n");
    print_separator();
    printf("  여름방학이 시작됩니다!\n");
    printf("  몸과 마음을 충전하는 시간...\n");
    print_separator();

    p->stamina  = p->max_stamina;
    p->stress  -= 30;
    if (p->stress < 0) p->stress = 0;
    REMOVE_STATUS(p, STATUS_TIRED);
    REMOVE_STATUS(p, STATUS_SICK);
    REMOVE_STATUS(p, STATUS_STRESSED);

    /* 방학 알바 기회 */
    printf("\n  방학 동안 알바를 할까요?\n");
    printf("  1. 편의점 알바 (체력 -20, 스트레스 +10, 용돈 +80,000원)\n");
    printf("  2. 그냥 쉬기  (체력 완전 회복, 스트레스 -20 추가)\n");
    int choice = get_int_input("선택: ", 1, 2);
    if (choice == 1) {
        p->money   += 80000;
        p->stamina -= 20;
        p->stress  += 10;
        printf("  알바 완료! 용돈 +80,000원\n");
    } else {
        p->stamina  = p->max_stamina;
        p->stress  -= 20;
        if (p->stress < 0) p->stress = 0;
        printf("  충분히 쉬었다. 체력 완전 회복!\n");
    }
    press_enter();
}

/* ------------------------------------------------
   advance_month
   다음 달로 이동. 12월 기말 후 → 새 학년 3월
   ------------------------------------------------ */
void advance_month(Player *p) {
    p->month++;

    /* 8월 = 여름방학, 자동 스킵 후 9월로 */
    if (p->month == 8) {
        summer_vacation(p);
        p->month = 9;
        p->current_day    = 1;
        p->mock_exam_done = 0;
        printf("\n  ──── 2학기(9월)가 시작됩니다. ────\n");
        press_enter();
        return;
    }

    /* 12월 기말 후 → 새 학년 */
    if (p->month == 13) {
        p->month = 3;
        p->year++;
        p->age++;

        if (p->year <= 3) {
            const char *yn[] = {"", "고1", "고2", "고3"};
            printf("\n  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
            printf("  진급! %s이 되었습니다! (%d세)\n", yn[p->year], p->age);
            printf("  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
            p->max_stamina += 5;
            p->stamina      = p->max_stamina;
            p->stress       = 10;
            printf("  최대 체력 +5, 새 학기 체력 완전 회복!\n");
        }
        press_enter();
    } else {
        /* 일반 달 전환 */
        const char *mnames[] = {
            "","1월","2월","3월","4월","5월","6월",
            "7월","8월","9월","10월","11월","12월"
        };
        printf("\n  ──── %s이 시작됩니다. ────\n", mnames[p->month]);
        press_enter();
    }

    p->current_day    = 1;
    p->mock_exam_done = 0;
}

/* ------------------------------------------------
   month_has_mock_exam  /  month_has_midterm  / etc.
   ------------------------------------------------ */
static int month_has_mock(int month) {
    return (month == 3 || month == 6 || month == 9);
}
static int month_has_midterm(int month) {
    return (month == MIDTERM_1_MONTH || month == MIDTERM_2_MONTH);
}
static int month_has_final(int month) {
    return (month == FINAL_1_MONTH || month == FINAL_2_MONTH);
}

/* ------------------------------------------------
   run_game
   ------------------------------------------------ */
void run_game() {
    srand((unsigned int)time(NULL));

    Player  player;
    GameMap map;
    memset(&player, 0, sizeof(Player));
    init_map(&map);

    /* 타이틀 */
    clear_screen();
    print_title();
    printf("  1. 새 게임\n");
    printf("  2. 이어하기%s\n",       save_exists()     ? "" : "  (저장 없음)");
    printf("  3. 자동저장에서 불러오기%s\n", autosave_exists() ? "" : "  (자동저장 없음)");
    int start_choice = get_int_input("선택: ", 1, 3);

    if (start_choice == 2) {
        if (!load_game(&player)) {
            printf("저장 파일이 없습니다. 새 게임을 시작합니다.\n");
            press_enter();
            start_choice = 1;
        }
    } else if (start_choice == 3) {
        if (!load_autosave(&player)) {
            printf("자동저장 파일이 없습니다. 새 게임을 시작합니다.\n");
            press_enter();
            start_choice = 1;
        }
    }

    if (start_choice == 1) {
        char name[MAX_NAME_LEN];
        printf("\n캐릭터 이름을 입력하세요: ");
        scanf("%31s", name);
        while (getchar() != '\n');
        init_player(&player, name);
        printf("\n%s님, 고등학교 1학년에 오신 걸 환영합니다! (16세)\n", player.name);
        printf("3년 후 수능을 정복하세요!\n");
        press_enter();
    }

    /* ── 메인 루프: 고1(year=1) ~ 고3(year=3) ── */
    while (player.year <= 3) {

        if (HAS_STATUS(&player, STATUS_GAME_OVER)) break;

        /* ── 월별 루프: 3월~12월 (8월은 advance_month에서 처리) ── */
        while (player.month <= 12) {

            /* 일별 루프 */
            while (player.current_day <= DAYS_PER_MONTH) {
                int day = player.current_day;

                /* 수능: 고3 12월 마지막 날 */
                if (player.year == 3 && player.month == 12
                        && day == DAYS_PER_MONTH) {
                    run_map_day(&player, &map, day);   /* 마지막 날 자유행동 */
                    player.current_day++;
                    clear_screen();
                    print_separator();
                    printf("  드디어 수능 날이 왔습니다!\n");
                    printf("  지금까지의 모든 노력을 보여주세요!\n");
                    print_separator();
                    press_enter();
                    run_exam(&player, 3);
                    player.year = 4; /* 루프 종료 */
                    goto game_end;
                }

                /* 중간/기말/모의고사: 마지막 날 자유행동 먼저, 그 다음 시험 */
                if (day == DAYS_PER_MONTH) {
                    run_map_day(&player, &map, day);
                    player.current_day++;
                    autosave(&player);

                    if (month_has_mock(player.month) && !player.mock_exam_done) {
                        run_exam(&player, 0);
                        player.mock_exam_done = 1;
                    } else if (month_has_midterm(player.month)) {
                        run_exam(&player, 1);
                    } else if (month_has_final(player.month)) {
                        run_exam(&player, 2);
                    }
                    break;   /* 시험 후 다음 달로 */
                }

                run_map_day(&player, &map, day);
                player.current_day++;
                autosave(&player);   /* 매일 자동저장 */
            }

            if (player.year > 3) break;

            /* 달 넘기기 */
            advance_month(&player);
            init_map(&map);

            /* 12월 기말 후 year++ 됐으면 월 루프 탈출 */
            if (player.month == 3) break;
        }
    }

game_end:
    /* ── 엔딩 ── */
    clear_screen();
    print_separator();
    printf("  게임 종료!\n");
    printf("  학생: %s  /  최종 나이: %d세\n", player.name, player.age);
    printf("  총 공부일수: %d일\n", player.days_studied);
    printf("  최종 합산 점수: %d / %d\n",
           get_total_score(&player), NUM_SUBJECTS * 100);
    print_separator();
    print_subjects(&player);
}
