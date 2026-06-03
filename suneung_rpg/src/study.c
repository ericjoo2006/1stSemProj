#include "game.h"

/* ------------------------------------------------
   study_subject
   Study a specific subject for `hours` hours.
   Status flags affect XP gain.
   ------------------------------------------------ */
void study_subject(Player *p, int subject_index, int hours) {
    if (subject_index < 0 || subject_index >= NUM_SUBJECTS) return;

    Subject *s = &p->subjects[subject_index];

    /* Can't study if sick */
    if (HAS_STATUS(p, STATUS_SICK)) {
        printf("몸이 너무 아파서 공부할 수 없습니다...\n");
        return;
    }
    /* Stamina cost: 10 per hour */
    int stamina_cost = hours * 10;
    if (p->stamina < stamina_cost) {
        printf("체력이 부족합니다. 먼저 쉬세요!\n");
        return;
    }

    /* Calculate XP gain */
    int base_xp = hours * 15;

    /* Bit flag checks */
    if (HAS_STATUS(p, STATUS_TIRED))
        base_xp /= 2;           /* 피곤하면 절반 */
    if (HAS_STATUS(p, STATUS_FOCUSED))
        base_xp *= 2;           /* 집중하면 2배 */
    if (HAS_STATUS(p, STATUS_STRESSED))
        base_xp = (base_xp * 3) / 4;  /* 스트레스는 75% */

    /* Apply XP */
    s->xp += base_xp;
    p->stamina -= stamina_cost;
    p->stress  += hours * 3;
    p->days_studied++;

    printf("[%s] 공부 완료! +%d XP\n", s->name, base_xp);

    /* Level up loop: each 100 XP raises score by 1, but harder at higher scores */
    while (s->xp >= s->xp_to_next && s->score < 100) {
        s->xp       -= s->xp_to_next;
        s->score    += 1;
        /* XP required increases as score grows (harder to improve near 100) */
        s->xp_to_next = 100 + (s->score - 40) * 5;
        printf("  ★ %s 점수 상승! → %d점\n", s->name, s->score);
    }

    /* Update tired status */
    if (p->stamina <= 30)
        ADD_STATUS(p, STATUS_TIRED);
    if (p->stress >= 80)
        ADD_STATUS(p, STATUS_SICK);
}

/* ------------------------------------------------
   go_to_hagwon
   Pay money for guaranteed bonus XP
   ------------------------------------------------ */
void go_to_hagwon(Player *p, int subject_index) {
    if (subject_index < 0 || subject_index >= NUM_SUBJECTS) return;

    int cost = 30000;
    if (p->money < cost) {
        printf("용돈이 부족합니다. (필요: %d원)\n", cost);
        return;
    }
    if (HAS_STATUS(p, STATUS_SICK)) {
        printf("아파서 학원에 갈 수 없습니다.\n");
        return;
    }

    Subject *s = &p->subjects[subject_index];
    p->money   -= cost;

    int xp_gain = 50; /* guaranteed 50 XP */
    if (HAS_STATUS(p, STATUS_TIRED))
        xp_gain = 35; /* tired reduces benefit */

    s->xp    += xp_gain;
    p->stamina -= 15;
    p->stress  += 8;

    printf("[학원 - %s] 수업 완료! +%d XP (-%d원)\n",
           s->name, xp_gain, cost);

    while (s->xp >= s->xp_to_next && s->score < 100) {
        s->xp       -= s->xp_to_next;
        s->score    += 1;
        s->xp_to_next = 100 + (s->score - 40) * 5;
        printf("  ★ %s 점수 상승! → %d점\n", s->name, s->score);
    }

    if (p->stamina <= 30) ADD_STATUS(p, STATUS_TIRED);
    if (p->stress >= 80)  ADD_STATUS(p, STATUS_SICK);
}

/* ------------------------------------------------
   self_study
   Player chooses a subject to study (1 hour free)
   ------------------------------------------------ */
void self_study(Player *p) {
    if (HAS_STATUS(p, STATUS_SICK)) {
        printf("아파서 공부할 수 없습니다.\n");
        return;
    }
    print_subjects(p);

    printf("\n어떤 과목을 공부할까요?\n");
    for (int i = 0; i < NUM_SUBJECTS; i++)
        printf("  %d. %s\n", i + 1, p->subjects[i].name);

    int choice = get_int_input("선택 (1–6): ", 1, NUM_SUBJECTS);
    int hours  = get_int_input("몇 시간 공부? (1–4): ", 1, 4);

    study_subject(p, choice - 1, hours);
}

/* ------------------------------------------------
   rest
   Recover stamina and reduce stress
   ------------------------------------------------ */
void rest(Player *p) {
    printf("\n[쉬는 중...] 체력이 회복됩니다.\n");

    int stamina_gain = 30;
    int stress_loss  = 20;

    /* Energized buff gives bonus recovery */
    if (HAS_STATUS(p, STATUS_ENERGIZED)) {
        stamina_gain += 10;
        stress_loss  += 10;
    }

    p->stamina += stamina_gain;
    p->stress  -= stress_loss;

    if (p->stamina > p->max_stamina) p->stamina = p->max_stamina;
    if (p->stress < 0)               p->stress  = 0;

    printf("  체력 +%d, 스트레스 -%d\n", stamina_gain, stress_loss);

    /* Clear tired flag if stamina recovered */
    if (p->stamina > 30)
        REMOVE_STATUS(p, STATUS_TIRED);
    if (p->stress < 80)
        REMOVE_STATUS(p, STATUS_SICK);
}

/* ------------------------------------------------
   visit_pc_bang
   Spend money and time to recover stress
   Risk: might get even more stressed if you play too long
   ------------------------------------------------ */
void visit_pc_bang(Player *p) {
    int cost = 3000;
    if (p->money < cost) {
        printf("용돈이 부족합니다!\n");
        return;
    }

    printf("\n[PC방에 갔습니다!]\n");
    int hours = get_int_input("몇 시간 플레이? (1–3): ", 1, 3);

    p->money   -= cost * hours;
    p->stress  -= hours * 15;
    p->stamina -= hours * 5;

    if (p->stress < 0)  p->stress  = 0;
    if (p->stamina < 0) p->stamina = 0;

    printf("  스트레스 -%d, 체력 -%d, 지출 -%d원\n",
           hours * 15, hours * 5, cost * hours);

    /* Over 2 hours: risk of becoming even more stressed tomorrow */
    if (hours >= 3) {
        printf("  너무 오래 놀았다... 내일 죄책감으로 스트레스가 올라갈 것 같다.\n");
        ADD_STATUS(p, STATUS_STRESSED);
    }

    REMOVE_STATUS(p, STATUS_TIRED);
    if (p->stress < 80) REMOVE_STATUS(p, STATUS_SICK);
}
