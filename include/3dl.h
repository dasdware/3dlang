#ifndef __3DL_H
#define __3DL_H

#include <arena.h>
#include <dw_array.h>
#include <error.h>
#include <stdbool.h>

#define TD_FOREACH(board, cursor) \
    for (TD_BoardCursor cursor = td_cursor_first(board); cursor.valid; cursor = td_cursor_next(cursor))

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

struct _TD_BoardHistory;

typedef struct
{
    TD_Cell *cells;
    struct _TD_BoardHistory *history;
    int result;
    TD_Status status;
    size_t time;
} TD_Board;

typedef struct
{
    TD_Board* board;
    int row;
    int col;
    TD_Cell* cell;
    bool valid;
} TD_BoardCursor;

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

    bool loaded;

    Arena cells_arena;
} TD_BoardHistory;

typedef struct
{
    TD_BoardCursor timewarp_cursor;
    TD_BoardCursor cell_cursor;
    int value;
    int dt;
} TD_Timewarp;

typedef da_array(TD_Timewarp) TD_Timewarps;

// Enum operations
const char* td_cell_kind_name(TD_CellKind kind);
const char* td_status_name(TD_Status status);

// Loading / Freeing
void td_load(TD_BoardHistory* history, const char* board_def, int input_a, int input_b);
void td_read(TD_BoardHistory* history, const char* filename, int input_a, int input_b);
void td_free(TD_BoardHistory* history);

// History navigation
TD_Board* td_current_board(TD_BoardHistory* history);
void td_forward(TD_BoardHistory* history);
void td_back(TD_BoardHistory* history);
void td_fast_forward(TD_BoardHistory* history);
void td_rewind(TD_BoardHistory* history);
void td_reset(TD_BoardHistory* history, int input_a, int input_b);

// Cursor operations
TD_BoardCursor td_cursor_first(TD_Board* board);
TD_BoardCursor td_cursor_next(TD_BoardCursor cursor);

TD_BoardCursor td_cursor_board(TD_BoardCursor cursor, TD_Board* board);

TD_BoardCursor td_cursor_move(TD_BoardCursor cursor, int cols, int rows);
#define td_cursor_left(cursor) td_cursor_move(cursor, -1, 0)
#define td_cursor_right(cursor) td_cursor_move(cursor, 1, 0)
#define td_cursor_up(cursor) td_cursor_move(cursor, 0, -1)
#define td_cursor_down(cursor) td_cursor_move(cursor, 0, 1)

bool td_cursor_same(TD_BoardCursor first, TD_BoardCursor second);

#endif // __3DL_H