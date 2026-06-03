#include "game.h"

/* ------------------------------------------------
   create_boss
   Factory function for all exam bosses
   year 1–3 = 중학교, 4–6 = 고등학교
   is_midterm: 1=중간/기말, 0=모의고사, 2=수능
   ------------------------------------------------ */
Boss create_boss(int year, int semester, int is_midterm) {
    Boss b;
    memset(&b, 0, sizeof(Boss));
    b.year_required = year;
    b.is_final_boss = 0;

    /* 수능 — final boss */
    if (year == 6 && is_midterm == 2) {
        strcpy(b.name, "★ 대학수학능력시험 (수능) ★");
        b.max_hp    = 500;
        b.attack    = 30;
        b.difficulty= 10;
        b.is_final_boss = 1;
    }
    /* 모의고사 (year 4–6 only) */
    else if (is_midterm == 0) {
        const char *mock_names[] = {"3월 전국모의고사", "6월 모의고사", "9월 모의고사"};
        int mock_idx = (semester <= 3) ? semester - 1 : 0;
        snprintf(b.name, sizeof(b.name), "[고%d] %s", year - 3, mock_names[mock_idx]);
        b.max_hp     = 150 + (year - 4) * 50 + mock_idx * 30;
        b.attack     = 10 + (year - 4) * 3 + mock_idx * 2;
        b.difficulty = 4 + (year - 4) * 2 + mock_idx;
    }
    /* 중간고사 / 기말고사 */
    else {
        const char *exam_type = (semester == 1) ? "중간고사" : "기말고사";
        const char *school = (year <= 3) ? "중" : "고";
        int grade = (year <= 3) ? year : year - 3;
        snprintf(b.name, sizeof(b.name), "[%s%d] %s", school, grade, exam_type);
        b.max_hp     = 80 + year * 20;
        b.attack     = 5 + year * 2;
        b.difficulty = 2 + year;
    }

    b.hp = b.max_hp;
    return b;
}

/* ------------------------------------------------
   print_boss_status
   ------------------------------------------------ */
void print_boss_status(const Boss *b) {
    int bar_len = 20;
    int filled  = (b->hp * bar_len) / b->max_hp;
    char bar[21];
    for (int i = 0; i < bar_len; i++)
        bar[i] = (i < filled) ? '#' : '.';
    bar[bar_len] = '\0';

    printf("\n  [%s]\n", b->name);
    printf("  HP: [%s] %d/%d\n", bar, b->hp, b->max_hp);
}

/* ------------------------------------------------
   run_battle
   Turn-based battle: player subjects vs boss
   Returns 1 if player wins, 0 if player loses
   ------------------------------------------------ */
int run_battle(Player *p, Boss *boss) {
    printf("\n");
    print_separator();
    printf("   시험이 시작됩니다!\n");
    printf("   적: %s\n", boss->name);
    print_separator();
    press_enter();

    /* Activate exam mode */
    ADD_STATUS(p, STATUS_EXAM_MODE);

    /* Player battle HP = average of subject scores */
    int total = get_total_score(p);
    p->hp = total / NUM_SUBJECTS;
    p->max_hp = 100;

    int round = 1;

    while (p->hp > 0 && boss->hp > 0) {
        printf("\n─── 라운드 %d ───\n", round);
        print_boss_status(boss);
        printf("  내 HP: %d/%d  |  스트레스: %d\n",
               p->hp, p->max_hp, p->stress);

        printf("\n행동을 선택하세요:\n");
        printf("  1. 문제 풀기 (과목 선택)\n");
        printf("  2. 아이템 사용\n");
        printf("  3. 찍기 (랜덤 데미지, 운에 맡긴다!)\n");

        int action = get_int_input("선택: ", 1, 3);

        int player_damage = 0;

        if (action == 1) {
            /* Choose which subject to "attack" with */
            printf("어떤 과목으로 풀까요?\n");
            for (int i = 0; i < NUM_SUBJECTS; i++)
                printf("  %d. %-10s (%d점)\n",
                       i + 1, p->subjects[i].name, p->subjects[i].score);
            int subj = get_int_input("선택 (1–6): ", 1, NUM_SUBJECTS) - 1;

            /* Damage = score/10 ± small variance */
            int base = p->subjects[subj].score / 10;
            int variance = (rand() % 3) - 1; /* -1, 0, or +1 */
            player_damage = base + variance;
            if (player_damage < 1) player_damage = 1;

            /* Status modifiers */
            if (HAS_STATUS(p, STATUS_FOCUSED))  player_damage += 2;
            if (HAS_STATUS(p, STATUS_STRESSED))  player_damage -= 1;
            if (HAS_STATUS(p, STATUS_SICK))      player_damage  = 1;

            printf("  [%s]으로 공격! 데미지: %d\n",
                   p->subjects[subj].name, player_damage);
        }
        else if (action == 2) {
            if (p->item_count == 0) {
                printf("  아이템이 없습니다!\n");
                /* Still boss's turn */
            } else {
                printf("아이템 선택:\n");
                for (int i = 0; i < p->item_count; i++)
                    printf("  %d. %s (x%d) — %s\n",
                           i + 1,
                           p->inventory[i].name,
                           p->inventory[i].quantity,
                           p->inventory[i].description);
                int item_idx = get_int_input("선택: ", 1, p->item_count) - 1;
                use_item(p, item_idx);
                player_damage = 0; /* no attack this turn */
            }
        }
        else {
            /* 찍기: pure random */
            player_damage = rand() % 15; /* 0–14 */
            if (player_damage == 0) {
                printf("  완전히 틀렸다...! 데미지: 0 (오히려 자신감 ↓)\n");
                p->stress += 5;
            } else {
                printf("  찍었는데 맞았다?! 데미지: %d\n", player_damage);
            }
        }

        boss->hp -= player_damage;
        if (boss->hp < 0) boss->hp = 0;

        /* Boss counterattack */
        if (boss->hp > 0) {
            int boss_dmg = boss->attack + (rand() % 5) - 2;
            if (boss_dmg < 1) boss_dmg = 1;

            /* Stress makes boss hit harder */
            if (p->stress >= 80) boss_dmg += 5;

            p->hp -= boss_dmg;
            if (p->hp < 0) p->hp = 0;

            printf("  [%s]이 반격! 피해: %d  (남은 HP: %d)\n",
                   boss->name, boss_dmg, p->hp);
        }

        round++;
        press_enter();
    }

    REMOVE_STATUS(p, STATUS_EXAM_MODE);

    if (p->hp <= 0) {
        printf("\n\n  ╔══════════════════════════════╗\n");
        printf("  ║   시험에서 졌습니다...         ║\n");
        printf("  ║   다음엔 더 열심히 하자!        ║\n");
        printf("  ╚══════════════════════════════╝\n");
        p->stress += 20;
        return 0;
    } else {
        printf("\n\n  ╔══════════════════════════════╗\n");
        printf("  ║   시험을 통과했습니다!  🎉      ║\n");
        printf("  ║   수고했어요!                   ║\n");
        printf("  ╚══════════════════════════════╝\n");

        /* Reward: money and stress relief */
        int reward = 10000 + boss->difficulty * 5000;
        p->money  += reward;
        p->stress -= 10;
        if (p->stress < 0) p->stress = 0;

        printf("  용돈 +%d원을 받았습니다!\n", reward);

        /* Final boss win */
        if (boss->is_final_boss) {
            printf("\n\n");
            print_separator();
            printf("  ★★★ 수능을 정복했습니다! ★★★\n");
            printf("  당신의 6년간의 노력이 빛을 발했습니다.\n");
            int final_score = get_total_score(p);
            printf("  최종 합산 점수: %d / %d\n", final_score, NUM_SUBJECTS * 100);
            print_separator();
        }
        return 1;
    }
}
