#ifndef __3DL_H
#define __3DL_H

#include <arena.h>
#include <error.h>
#include <stdbool.h>

typedef enum
{
    CELL_EMPTY,
    CELL_NUMBER,
    CELL_MOVE_LEFT,
    CELL_MOVE_RIGHT,
    CELL_MOVE_UP,
    CELL_MOVE_DOWN,
    CELL_CALC_ADD,
    CELL_CALC_SUBTRACT,
    CELL_CALC_DIVIDE,
    CELL_CALC_MULTIPLY,
    CELL_CALC_REMAINDER,
    CELL_CMP_EQUAL,
    CELL_CMP_NOTEQUAL,
    CELL_TIMEWARP,
    CELL_STOP,
} TD_CellKind;

typedef enum
{
    CELL_INPUT_NONE,
    CELL_INPUT_A,
    CELL_INPUT_B,
} TD_CellInputKind;

typedef enum
{
    STATUS_OK,
    STATUS_CRASH,
    STATUS_RUNNING,
    STATUS_STOPPED,
    STATUS_STALLED,
} TD_Status;

typedef struct
{
    TD_CellKind kind;
    TD_CellInputKind input_kind;
    int value;
    bool active;
} TD_Cell;

typedef struct
{
    TD_Cell *items;
    size_t capacity;
    size_t count;
} TD_CellList;

struct _TD_BoardHistory;

typedef struct
{
    TD_Cell *cells;
    struct _TD_BoardHistory *history;
    int result;
    TD_Status status;
    size_t time;
} TD_Board;

typedef struct _TD_BoardHistory
{
    size_t cols;
    size_t rows;
    size_t cells_bytes;

    TD_Board *items;
    size_t capacity;
    size_t count;

    size_t tick;
    int input_a;
    int input_b;

    Arena cells_arena;
} TD_BoardHistory;

typedef struct
{
    int tw_row;
    int tw_col;
    int row;
    int col;
    int value;
    int dt;
} TD_Timewarp;

typedef struct
{
    TD_Timewarp *items;
    size_t capacity;
    size_t count;
} TD_Timewarps;

// Enum operations
const char* td_cell_kind_name(TD_CellKind kind);
const char* td_status_name(TD_Status status);

// Loading / Freeing
void td_load(TD_BoardHistory* history, const char* board_def, int input_a, int input_b);
void td_read(TD_BoardHistory* history, const char* filename, int input_a, int input_b);
void td_free(TD_BoardHistory* history);

// Cell operations
TD_Cell make_empty_cell();
TD_Cell make_number_cell(int value);
TD_Cell* td_cell_at(TD_Board* board, int col, int row);
void td_set_cell(TD_Board* board, int col, int row, TD_Cell value);
void td_activate_cell(TD_Board* board, int col, int row);

// History navigation
TD_Board* td_current_board(TD_BoardHistory* history);
void td_forward(TD_BoardHistory* history);
void td_back(TD_BoardHistory* history);
void td_fast_forward(TD_BoardHistory* history);
void td_rewind(TD_BoardHistory* history);
void td_reset(TD_BoardHistory* history, int input_a, int input_b);

#endif // __3DL_H