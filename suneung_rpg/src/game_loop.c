#include "game.h"

/*
   School year structure:
   year 1–3 = 중1, 중2, 중3
   year 4–6 = 고1, 고2, 고3

   Each year has 2 semesters.
   Each semester has ~90 "days" of study time,
   then a 중간고사/기말고사 battle.
   Years 4–6 also have 모의고사 bosses mid-semester.
*/

#define DAYS_PER_SEMESTER  15  /* compressed for playability */
#define MOCK_EXAM_DAY      8   /* 모의고사 appears on day 8 of 고교 semesters */

/* ------------------------------------------------
   run_exam
   Trigger an exam boss fight
   ------------------------------------------------ */
static void run_exam(Player *p, int is_midterm) {
    clear_screen();
    printf("\n");
    print_separator();
    printf("  !! 시험 날입니다 !!\n");
    print_separator();

    Boss boss = create_boss(p->year, p->semester, is_midterm);
    int result = run_battle(p, &boss);

    p->total_score = get_total_score(p);

    if (!result) {
        /* Failed: small stat penalty */
        for (int i = 0; i < NUM_SUBJECTS; i++) {
            p->subjects[i].score -= 2;
            if (p->subjects[i].score < 0) p->subjects[i].score = 0;
        }
        printf("  ※ 모든 과목 점수 -2점 (좌절 패널티)\n");
    }

    press_enter();
}

/* ------------------------------------------------
   advance_to_next_period
   Move to next semester or next year
   ------------------------------------------------ */
void advance_to_next_period(Player *p) {
    if (p->semester == 1) {
        p->semester = 2;
        printf("\n  ──── 2학기가 시작됩니다. ────\n");
    } else {
        p->semester = 1;
        p->year++;
        const char *year_names[] = {"", "중1", "중2", "중3", "고1", "고2", "고3"};
        if (p->year <= 6)
            printf("\n  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
                   "  진학! %s이 되었습니다!\n"
                   "  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n",
                   year_names[p->year]);
        /* Stat boost on new year */
        p->max_stamina += 5;
        p->stamina = p->max_stamina;
        p->stress  = 20;
        printf("  최대 체력 +5, 체력 완전 회복!\n");
    }
    press_enter();
}

/* ------------------------------------------------
   run_game
   Main game loop
   ------------------------------------------------ */
void run_game() {
    srand((unsigned int)time(NULL));

    Player  player;
    GameMap map;
    memset(&player, 0, sizeof(Player));
    init_map(&map);

    /* Title screen */
    clear_screen();
    print_title();
    printf("  1. 새 게임\n");
    printf("  2. 이어하기\n");
    int start_choice = get_int_input("선택: ", 1, 2);

    if (start_choice == 2) {
        if (!load_game(&player)) {
            printf("저장 파일이 없습니다. 새 게임을 시작합니다.\n");
            start_choice = 1;
        }
    }

    if (start_choice == 1) {
        char name[MAX_NAME_LEN];
        printf("\n캐릭터 이름을 입력하세요: ");
        scanf("%31s", name);
        while (getchar() != '\n');
        init_player(&player, name);
        printf("\n%s님, 중학교 1학년에 오신 걸 환영합니다!\n", player.name);
        printf("6년 후 수능을 정복하세요!\n");
        press_enter();
    }

    /* ── Main game loop ── */
    /* year 1–6, each year has 2 semesters */
    while (player.year <= 6) {

        /* Skip if somehow status is game over */
        if (HAS_STATUS(&player, STATUS_GAME_OVER)) break;

        int is_highschool = (player.year >= 4);

        /* Study days loop */
        while (player.current_day <= DAYS_PER_SEMESTER) {
            int day = player.current_day;

            /* 모의고사 mid-semester (고교 only) */
            if (is_highschool && day == MOCK_EXAM_DAY
                    && player.semester <= 2 && !player.mock_exam_done) {
                (void)(player.semester);
                printf("\n  [모의고사가 있습니다!]\n");
                run_exam(&player, 0);
                player.mock_exam_done = 1;  /* 이번 학기 모의고사 완료 표시 */
            }

            /* Final boss: 수능 (고3, 2학기 마지막 날) */
            if (player.year == 6 && player.semester == 2
                    && day == DAYS_PER_SEMESTER) {
                clear_screen();
                printf("\n");
                print_separator();
                printf("  드디어 수능 날이 왔습니다!\n");
                printf("  지금까지의 모든 노력을 보여주세요!\n");
                print_separator();
                press_enter();
                run_exam(&player, 2); /* is_midterm=2 means 수능 */
                player.year = 7;
                break;
            }

            run_map_day(&player, &map, day);
            player.current_day++;
        }

        if (player.year > 6) break;

        /* End of semester exam (중간고사 or 기말고사) */
        run_exam(&player, player.semester); /* 1=중간, 2=기말 */

        /* Advance to next period */
        advance_to_next_period(&player);
        player.current_day    = 1;   /* 새 학기 날짜 초기화 */
        player.mock_exam_done = 0;   /* 새 학기 모의고사 플래그 초기화 */

        /* Reset map so player starts from home each semester */
        init_map(&map);
    }

    /* ── Ending ── */
    clear_screen();
    print_separator();
    printf("  게임 종료!\n");
    printf("  학생: %s\n", player.name);
    printf("  총 공부일수: %d일\n", player.days_studied);
    printf("  최종 합산 점수: %d / %d\n",
           get_total_score(&player), NUM_SUBJECTS * 100);
    print_separator();
    print_subjects(&player);
}
