#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ================================================
   CONSTANTS
   ================================================ */
#define MAX_NAME_LEN     32
#define NUM_SUBJECTS     6
#define MAX_ITEMS        10
#define SAVE_FILE        "saves/save.dat"

/* Subjects index */
#define SUBJ_KOREAN      0
#define SUBJ_MATH        1
#define SUBJ_ENGLISH     2
#define SUBJ_SCIENCE     3
#define SUBJ_SOCIAL      4
#define SUBJ_SECOND_LANG 5

/* ================================================
   BIT MASK: Player Status Flags
   Use bitwise OR to combine, AND to check
   ================================================ */
#define STATUS_NORMAL    0x00   /* 정상 */
#define STATUS_TIRED     0x01   /* 피곤함  - studying gives less XP */
#define STATUS_SICK      0x02   /* 병남    - can't study */
#define STATUS_FOCUSED   0x04   /* 집중    - studying gives double XP */
#define STATUS_STRESSED  0x08   /* 스트레스 - stats temporarily reduced */
#define STATUS_ENERGIZED 0x10   /* 에너지  - all actions cost less stamina */
#define STATUS_EXAM_MODE 0x20   /* 시험 모드 - active during exam battles */
#define STATUS_GAME_OVER 0x40   /* 게임오버 */

/* Macro helpers for bit flags */
#define HAS_STATUS(p, flag)    ((p)->status_flags & (flag))
#define ADD_STATUS(p, flag)    ((p)->status_flags |= (flag))
#define REMOVE_STATUS(p, flag) ((p)->status_flags &= ~(flag))
#define CLEAR_STATUS(p)        ((p)->status_flags = STATUS_NORMAL)

/* ================================================
   MAP CONSTANTS  (2D array)
   ================================================ */
#define MAP_ROWS     8
#define MAP_COLS     10

/* Tile type codes stored in the 2D map grid */
#define TILE_EMPTY      '.'
#define TILE_WALL       '#'
#define TILE_ROAD       ' '
#define TILE_HOME       'H'   /* 집         - rest */
#define TILE_SCHOOL     'S'   /* 학교       - self_study */
#define TILE_HAGWON     'A'   /* 학원       - go_to_hagwon */
#define TILE_PCBANG     'P'   /* PC방       - visit_pc_bang */
#define TILE_STORE      'C'   /* 편의점     - shop */
#define TILE_LIBRARY    'L'   /* 도서관     - bonus study */
#define TILE_PARK       'K'   /* 공원       - stress recovery */
#define TILE_STUDYROOM  'R'   /* 독서실     - focused study */
#define TILE_PLAYER     '@'   /* 플레이어   */

/* ================================================
   STRUCTS
   ================================================ */

/* Map struct - holds the 2D grid and player position */
typedef struct {
    char grid[MAP_ROWS][MAP_COLS];   /* the actual 2D tile array */
    int  player_row;
    int  player_col;
    char under_player;               /* tile hidden under the @ symbol */
} GameMap;

/* Individual subject */
typedef struct {
    char name[MAX_NAME_LEN];   /* e.g., "국어" */
    int  score;                /* 0–100 */
    int  xp;                   /* accumulated study XP */
    int  xp_to_next;           /* XP needed to raise score by 1 */
} Subject;

/* Item struct */
typedef struct {
    char name[MAX_NAME_LEN];
    char description[64];
    int  stamina_restore;
    int  stress_reduce;
    int  study_bonus;          /* temporary bonus to study gain */
    int  quantity;
} Item;

/* Player struct */
typedef struct {
    char    name[MAX_NAME_LEN];
    int     year;              /* 1–6: 중1,중2,중3,고1,고2,고3 */
    int     semester;          /* 1 or 2 */
    int     stamina;           /* 0–100; reaches 0 → STATUS_TIRED */
    int     max_stamina;
    int     stress;            /* 0–100; reaches 100 → STATUS_SICK */
    int     hp;                /* battle HP */
    int     max_hp;
    int     money;             /* 용돈 */
    unsigned int status_flags; /* bit mask */
    Subject subjects[NUM_SUBJECTS];
    Item    inventory[MAX_ITEMS];
    int     item_count;
    int     days_studied;      /* total days studied */
    int     total_score;       /* last exam total */
    int     current_day;       /* current day within semester (1~DAYS_PER_SEMESTER) */
} Player;

/* Boss (exam) struct */
typedef struct {
    char name[64];             /* e.g., "3월 모의고사" */
    int  hp;
    int  max_hp;
    int  attack;               /* base damage per round */
    int  difficulty;           /* 1–10 */
    int  year_required;        /* which school year this appears */
    int  is_final_boss;        /* 1 if 수능 */
} Boss;

/* ================================================
   FUNCTION PROTOTYPES
   ================================================ */

/* player.c */
void   init_player(Player *p, const char *name);
void   print_player_status(const Player *p);
void   print_subjects(const Player *p);
void   add_item(Player *p, Item item);
void   use_item(Player *p, int item_index);
int    get_total_score(const Player *p);
void   apply_day_end(Player *p);

/* study.c */
void   study_subject(Player *p, int subject_index, int hours);
void   go_to_hagwon(Player *p, int subject_index);
void   self_study(Player *p);
void   rest(Player *p);
void   visit_pc_bang(Player *p);

/* battle.c */
int    run_battle(Player *p, Boss *boss);
Boss   create_boss(int year, int semester, int is_midterm);
void   print_boss_status(const Boss *b);

/* file_io.c */
int    save_game(const Player *p);
int    load_game(Player *p);

/* ui.c */
void   print_title();
void   clear_screen();
void   print_separator();
void   print_status_flags(unsigned int flags);
int    get_int_input(const char *prompt, int min, int max);
void   press_enter();

/* map.c */
void   init_map(GameMap *m);
void   print_map(const GameMap *m);
int    move_player(GameMap *m, char direction);  /* returns tile landed on */
void   run_map_day(Player *p, GameMap *m, int day);

/* game_loop.c */
void   run_game();
void   advance_to_next_period(Player *p);

#endif /* GAME_H */
