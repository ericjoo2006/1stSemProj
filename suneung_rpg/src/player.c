#include "game.h"

/* Subject names in Korean */
static const char *SUBJECT_NAMES[NUM_SUBJECTS] = {
    "국어", "수학", "영어", "과학", "사회", "제2외국어"
};

/* ------------------------------------------------
   init_player
   Sets up a brand new player at 중학교 1학년
   ------------------------------------------------ */
void init_player(Player *p, const char *name) {
    memset(p, 0, sizeof(Player));

    strncpy(p->name, name, MAX_NAME_LEN - 1);
    p->year         = 1;
    p->semester     = 1;
    p->stamina      = 100;
    p->max_stamina  = 100;
    p->stress       = 0;
    p->hp           = 100;
    p->max_hp       = 100;
    p->money        = 50000;   /* 용돈 5만원 */
    p->status_flags = STATUS_NORMAL;
    p->item_count   = 0;
    p->days_studied = 0;
    p->total_score  = 0;
    p->current_day  = 1;
    p->mock_exam_done = 0;

    /* Init each subject */
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        strncpy(p->subjects[i].name, SUBJECT_NAMES[i], MAX_NAME_LEN - 1);
        p->subjects[i].score       = 40 + (rand() % 20); /* start 40–59 */
        p->subjects[i].xp          = 0;
        p->subjects[i].xp_to_next  = 100;
    }

    /* Give starter item: 에너지 드링크 x2 */
    Item starter;
    strncpy(starter.name, "에너지 드링크", MAX_NAME_LEN - 1);
    strncpy(starter.description, "피로를 20 회복합니다.", 63);
    starter.stamina_restore = 20;
    starter.stress_reduce   = 0;
    starter.study_bonus     = 0;
    starter.quantity        = 2;
    add_item(p, starter);
}

/* ------------------------------------------------
   print_player_status
   ------------------------------------------------ */
void print_player_status(const Player *p) {
    const char *year_names[] = {
        "", "중1", "중2", "중3", "고1", "고2", "고3"
    };
    printf("\n╔══════════════════════════════════════╗\n");
    printf("║  학생: %-10s  학년: %-4s %d학기 ║\n",
           p->name,
           (p->year >= 1 && p->year <= 6) ? year_names[p->year] : "??",
           p->semester);
    printf("╠══════════════════════════════════════╣\n");
    printf("║  HP:      %3d / %3d                  ║\n", p->hp, p->max_hp);
    printf("║  체력:    %3d / %3d                  ║\n", p->stamina, p->max_stamina);
    printf("║  스트레스:%3d / 100                  ║\n", p->stress);
    printf("║  용돈:    %6d원                    ║\n", p->money);
    printf("║  상태: ");
    print_status_flags(p->status_flags);
    printf("╚══════════════════════════════════════╝\n");
}

/* ------------------------------------------------
   print_subjects
   ------------------------------------------------ */
void print_subjects(const Player *p) {
    printf("\n[과목 성적]\n");
    printf("%-12s  점수   XP 진행\n", "과목");
    printf("────────────────────────────\n");
    for (int i = 0; i < NUM_SUBJECTS; i++) {
        const Subject *s = &p->subjects[i];
        /* Draw a small XP bar */
        int bar_filled = (s->xp * 10) / s->xp_to_next;
        char bar[11];
        for (int b = 0; b < 10; b++)
            bar[b] = (b < bar_filled) ? '#' : '-';
        bar[10] = '\0';
        printf("%-12s  %3d점  [%s] %d/%d\n",
               s->name, s->score, bar, s->xp, s->xp_to_next);
    }
}

/* ------------------------------------------------
   add_item
   ------------------------------------------------ */
void add_item(Player *p, Item item) {
    /* Check if we already have this item */
    for (int i = 0; i < p->item_count; i++) {
        if (strcmp(p->inventory[i].name, item.name) == 0) {
            p->inventory[i].quantity += item.quantity;
            return;
        }
    }
    if (p->item_count < MAX_ITEMS) {
        p->inventory[p->item_count] = item;
        p->item_count++;
    } else {
        printf("가방이 꽉 찼습니다!\n");
    }
}

/* ------------------------------------------------
   use_item
   ------------------------------------------------ */
void use_item(Player *p, int idx) {
    if (idx < 0 || idx >= p->item_count) {
        printf("잘못된 아이템 번호입니다.\n");
        return;
    }
    Item *item = &p->inventory[idx];
    if (item->quantity <= 0) {
        printf("아이템이 없습니다.\n");
        return;
    }

    printf("[%s] 사용!\n", item->name);

    /* Apply effects */
    if (item->stamina_restore > 0) {
        p->stamina += item->stamina_restore;
        if (p->stamina > p->max_stamina) p->stamina = p->max_stamina;
        printf("  체력 +%d\n", item->stamina_restore);
    }
    if (item->stress_reduce > 0) {
        p->stress -= item->stress_reduce;
        if (p->stress < 0) p->stress = 0;
        printf("  스트레스 -%d\n", item->stress_reduce);
    }
    if (item->study_bonus > 0) {
        ADD_STATUS(p, STATUS_FOCUSED);
        printf("  집중력 버프 획득!\n");
    }

    /* Update status flags based on new values */
    if (p->stamina > 30 && HAS_STATUS(p, STATUS_TIRED))
        REMOVE_STATUS(p, STATUS_TIRED);
    if (p->stress < 80 && HAS_STATUS(p, STATUS_SICK))
        REMOVE_STATUS(p, STATUS_SICK);

    item->quantity--;
    /* Remove item if quantity hits 0 */
    if (item->quantity == 0) {
        for (int i = idx; i < p->item_count - 1; i++)
            p->inventory[i] = p->inventory[i + 1];
        p->item_count--;
    }
}

/* ------------------------------------------------
   get_total_score  — sum of all subject scores
   ------------------------------------------------ */
int get_total_score(const Player *p) {
    int total = 0;
    for (int i = 0; i < NUM_SUBJECTS; i++)
        total += p->subjects[i].score;
    return total;
}

/* ------------------------------------------------
   apply_day_end
   Called at end of each in-game day
   ------------------------------------------------ */
void apply_day_end(Player *p) {
    /* Natural stress recovery overnight */
    p->stress -= 5;
    if (p->stress < 0) p->stress = 0;

    /* Stamina partial recovery */
    p->stamina += 20;
    if (p->stamina > p->max_stamina) p->stamina = p->max_stamina;

    /* Update status flags */
    if (p->stamina <= 30)
        ADD_STATUS(p, STATUS_TIRED);
    else
        REMOVE_STATUS(p, STATUS_TIRED);

    if (p->stress >= 80)
        ADD_STATUS(p, STATUS_SICK);
    else
        REMOVE_STATUS(p, STATUS_SICK);

    /* Remove focused buff after each day */
    REMOVE_STATUS(p, STATUS_FOCUSED);
    REMOVE_STATUS(p, STATUS_ENERGIZED);
}
