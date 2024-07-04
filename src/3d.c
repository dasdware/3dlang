#include <stdio.h>
#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#define NOB_IMPLEMENTATION
#include <nob.h>

#define ARENA_IMPLEMENTATION
#include <arena.h>

#include <error.h>

typedef enum {
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

typedef enum {
    CELL_INPUT_NONE,
    CELL_INPUT_A,
    CELL_INPUT_B,
} TD_CellInputKind;

typedef enum {
    STATUS_OK,
    STATUS_CRASH,
    STATUS_RUNNING,
    STATUS_STOPPED,
    STATUS_STALLED,
} TD_Status;

const char* cell_kind_name(TD_CellKind kind) {
    switch (kind) {
    case CELL_EMPTY:
        return "EMPTY";
    case CELL_NUMBER:
        return "NUMBER";
    case CELL_MOVE_LEFT:
        return "MOVE_LEFT";
    case CELL_MOVE_RIGHT:
        return "MOVE_RIGHT";
    case CELL_MOVE_UP:
        return "MOVE_UP";
    case CELL_MOVE_DOWN:
        return "MOVE_DOWN";
    case CELL_CALC_ADD:
        return "CALC_ADD";
    case CELL_CALC_SUBTRACT:
        return "CALC_SUBTRACT";
    case CELL_CALC_DIVIDE:
        return "CALC_DIVIDE";
    case CELL_CALC_MULTIPLY:
        return "CALC_MULTIPLY";
    case CELL_CALC_REMAINDER:
        return "CALC_REMAINDER";
    case CELL_CMP_EQUAL:
        return "CMP_EQUAL";
    case CELL_CMP_NOTEQUAL:
        return "CMP_NOTEQUAL";
    case CELL_TIMEWARP:
        return "TIMEWARP";
    case CELL_STOP:
        return "STOP";
    default:
        DW_UNIMPLEMENTED_MSG("Cannot retrieve cell kind name for `%d`.", kind);
    }
}

const char* status_name(TD_Status status) {
    switch (status) {
    case STATUS_OK:
        return "Ok";
    case STATUS_CRASH:
        return "Crashed";
    case STATUS_RUNNING:
        return "Running";
    case STATUS_STOPPED:
        return "Stopped";
    case STATUS_STALLED:
        return "Stalled";
    default:
        DW_UNIMPLEMENTED_MSG("Cannot retrieve status name for `%d`.", status);
    }
}

typedef struct {
    TD_CellKind kind;
    TD_CellInputKind input_kind;
    int value;
    bool active;
} TD_Cell;

typedef struct {
    TD_Cell* items;
    size_t capacity;
    size_t count;
} TD_CellList;

struct _TD_BoardHistory;

typedef struct {
    TD_Cell* cells;
    struct _TD_BoardHistory *history;
    int result;
    TD_Status status;
    size_t time;
}  TD_Board;

typedef struct _TD_BoardHistory {
    size_t cols;
    size_t rows;
    size_t cells_bytes;

    TD_Board* items;
    size_t capacity;
    size_t count;

    size_t tick;
    int input_a;
    int input_b;

    Arena cells_arena;
} TD_BoardHistory;

typedef struct {
    int row;
    int col;
    int value;
    int dt;
} TD_Timewarp;

typedef struct {
    TD_Timewarp* items;
    size_t capacity;
    size_t count;
} TD_Timewarps;

#define nob_sv_advance(sv) \
    do {                   \
        (sv).data++;       \
        (sv).count--;      \
    } while (false)

void td_load(TD_BoardHistory* history, const char* board_def, int input_a, int input_b)
{
    history->input_a = input_a;
    history->input_b = input_b;

    Nob_String_View orig_board_description = nob_sv_from_cstr(board_def);
    Nob_String_View board_description = orig_board_description;

    TD_Board first_board = {0};
    first_board.history = history;
    first_board.status = STATUS_RUNNING;
    first_board.result = 0;
    first_board.time = 1;

    TD_CellList all_cells = {0};
    while (board_description.count > 0) {
        Nob_String_View line = nob_sv_trim_left(nob_sv_chop_by_delim(&board_description, '\n'));
        if (line.count == 0) {
            continue;
        }

        history->rows++;

        size_t cols = 0;
        while (line.count > 0) {
            TD_Cell cell = {0};

            if (isdigit(line.data[0]) || (line.data[0] == '-' && line.count > 1 && isdigit(line.data[1]))) {
                bool negative = line.data[0] == '-';
                if (negative) {
                    nob_sv_advance(line);
                }

                int n = 0;
                while (line.count > 0 && isdigit(line.data[0])) {
                    n = (n * 10) + (line.data[0] - 48);
                    nob_sv_advance(line);
                }

                cell.kind = CELL_NUMBER;
                cell.value = (negative) ? -n : n;
            } else {
                switch (line.data[0]) {
                case '.':
                    cell.kind = CELL_EMPTY;
                    break;
                case '<':
                    cell.kind = CELL_MOVE_LEFT;
                    break;
                case '>':
                    cell.kind = CELL_MOVE_RIGHT;
                    break;
                case '^':
                    cell.kind = CELL_MOVE_UP;
                    break;
                case 'v':
                    cell.kind = CELL_MOVE_DOWN;
                    break;
                case '+':
                    cell.kind = CELL_CALC_ADD;
                    break;
                case '-':
                    cell.kind = CELL_CALC_SUBTRACT;
                    break;
                case '*':
                    cell.kind = CELL_CALC_MULTIPLY;
                    break;
                case '/':
                    cell.kind = CELL_CALC_DIVIDE;
                    break;
                case '%':
                    cell.kind = CELL_CALC_REMAINDER;
                    break;
                case '@':
                    cell.kind = CELL_TIMEWARP;
                    break;
                case '=':
                    cell.kind = CELL_CMP_EQUAL;
                    break;
                case '#':
                    cell.kind = CELL_CMP_NOTEQUAL;
                    break;
                case 'S':
                    cell.kind = CELL_STOP;
                    break;
                case 'A':
                    cell.kind = CELL_NUMBER;
                    cell.input_kind = CELL_INPUT_A;
                    cell.value = input_a;
                    break;
                case 'B':
                    cell.kind = CELL_NUMBER;
                    cell.input_kind = CELL_INPUT_B;
                    cell.value = input_b;
                    break;
                default:
                    DW_UNIMPLEMENTED_MSG("Unknwon cell symbol `%c`.", line.data[0]);
                }
                nob_sv_advance(line);
            }

            nob_da_append(&all_cells, cell);

            cols++;
            line = nob_sv_trim_left(line);
        }

        history->cols = max(history->cols, cols);
    }

    history->cells_bytes = history->cols * history->rows * sizeof(TD_Cell);
    first_board.cells = arena_alloc(&history->cells_arena, history->cells_bytes);
    memcpy(first_board.cells, all_cells.items, history->cells_bytes);

    nob_da_append(history, first_board);
}

void td_read(TD_BoardHistory* history, const char* filename, int input_a, int input_b) {
    *history = (TD_BoardHistory) {
        0
    };

    nob_log(NOB_INFO, "Loading program `%s`.", filename);

    Nob_String_Builder file = {0};
    nob_read_entire_file(filename, &file);
    nob_sb_append_null(&file);

    td_load(history, file.items, input_a, input_b);
    nob_sb_free(file);
}

void td_free(TD_BoardHistory* history) {
    nob_da_free(*history);
    arena_free(&history->cells_arena);
}

TD_Cell* td_cell_at(TD_Board* board, int col, int row) {
    static TD_Cell empty_cell = {0};
    if (col >= 0 && col < (int)board->history->cols && row >= 0 && row < (int)board->history->rows) {
        return &board->cells[row * board->history->cols + col];
    }

    return &empty_cell;
}

void td_set_cell(TD_Board* board, int col, int row, TD_Cell value) {
    TD_Cell* target = td_cell_at(board, col, row);
    TD_CellInputKind old_input_kind = target->input_kind;
    bool stopped = target->kind == CELL_STOP;

    *target = value;
    target->input_kind = old_input_kind;
    if (stopped) {
        board->status = STATUS_STOPPED;
        board->result = value.value;
    }
}

void td_activate_cell(TD_Board* board, int col, int row) {
    td_cell_at(board, col, row)->active = true;
}

TD_Cell make_empty_cell() {
    return (TD_Cell) {
        0
    };
}

TD_Cell make_number_cell(int value) {
    return (TD_Cell) {
        .kind = CELL_NUMBER,
        .value = value,
    };
}

bool td_retrieve_operands(TD_Board* board, size_t col, size_t row, TD_Cell** left, TD_Cell** right) {
    *left = td_cell_at(board, col - 1, row);
    *right = td_cell_at(board, col, row - 1);
    return (*left)->kind == CELL_NUMBER
           && (*right)->kind == CELL_NUMBER;
}

bool td_retrieve_timewarp_operands(TD_Board* board, size_t col, size_t row,
                                   TD_Cell** op_v, TD_Cell** op_dx, TD_Cell** op_dy, TD_Cell** op_dt) {
    *op_v = td_cell_at(board, col, row - 1);
    *op_dx = td_cell_at(board, col - 1, row);
    *op_dy = td_cell_at(board, col + 1, row);
    *op_dt = td_cell_at(board, col, row + 1);
    return (*op_v)->kind == CELL_NUMBER
           && (*op_dx)->kind == CELL_NUMBER
           && (*op_dy)->kind == CELL_NUMBER
           && (*op_dt)->kind == CELL_NUMBER;
}

void td_calculate(TD_Board* board, size_t col, size_t row, int value) {
    td_set_cell(board, col - 1, row, make_empty_cell());
    td_set_cell(board, col, row - 1, make_empty_cell());
    td_set_cell(board, col + 1, row, make_number_cell(value));
    td_set_cell(board, col, row + 1, make_number_cell(value));
    td_activate_cell(board, col, row);
    td_activate_cell(board, col + 1, row);
    td_activate_cell(board, col, row + 1);
}

void td_move_left(TD_Board* board, size_t col, size_t row, TD_Cell* operand) {
    td_set_cell(board, col + 1, row, make_empty_cell());
    td_set_cell(board, col - 1, row, *operand);
    td_activate_cell(board, col, row);
    td_activate_cell(board, col - 1, row);
}

void td_move_right(TD_Board* board, size_t col, size_t row, TD_Cell* operand) {
    td_set_cell(board, col - 1, row, make_empty_cell());
    td_set_cell(board, col + 1, row, *operand);
    td_activate_cell(board, col, row);
    td_activate_cell(board, col + 1, row);
}

void td_move_up(TD_Board* board, size_t col, size_t row, TD_Cell* operand) {
    td_set_cell(board, col, row + 1, make_empty_cell());
    td_set_cell(board, col, row - 1, *operand);
    td_activate_cell(board, col, row);
    td_activate_cell(board, col, row - 1);
}

void td_move_down(TD_Board* board, size_t col, size_t row, TD_Cell* operand) {
    td_set_cell(board, col, row - 1, make_empty_cell());
    td_set_cell(board, col, row + 1, *operand);
    td_activate_cell(board, col, row);
    td_activate_cell(board, col, row + 1);
}

TD_Board* td_current_board(TD_BoardHistory* history) {
    return &history->items[history->tick];
}

void _td_collect_timewarps(TD_Board* board, TD_Timewarps* timewarps) {
    TD_Cell *op_v, *op_dx, *op_dy, *op_dt;
    for (int col = 0; col < (int)board->history->cols; ++col) {
        for (int row = 0; row < (int)board->history->cols; ++row) {
            TD_Cell* current_cell = td_cell_at(board, col, row);
            switch (current_cell->kind) {
            case CELL_TIMEWARP: {
                if (td_retrieve_timewarp_operands(board, col, row, &op_v, &op_dx, &op_dy, &op_dt)) {
                    TD_Timewarp tw = {
                        .col = col - op_dx->value,
                        .row = row - op_dy->value,
                        .value = op_v->value,
                        .dt = op_dt->value,
                    };
                    nob_da_append(timewarps, tw);
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

TD_Board* td_clone_board(TD_BoardHistory *history, size_t index, int time) {
    TD_Board* board = &history->items[index];

    TD_Board new_board = {0};
    new_board.history = history;
    new_board.result = 0;
    new_board.status = STATUS_RUNNING;
    new_board.time = time;
    new_board.cells = arena_alloc(&history->cells_arena, history->cells_bytes);
    memcpy(new_board.cells, board->cells, history->cells_bytes);

    for (int col = 0; col < (int)history->cols; ++col) {
        for (int row = 0; row < (int)history->rows; ++row) {
            td_cell_at(&new_board, col, row)->active = false;
        }
    }

    nob_da_append(history, new_board);

    return &history->items[history->count - 1];
}

void td_crash(TD_BoardHistory* history) {
    TD_Board* current_board = td_current_board(history);
    TD_Board* next_board = td_clone_board(history, history->count - 1, current_board->time + 1);
    next_board->status = STATUS_CRASH;
}

void td_forward(TD_BoardHistory* history) {
    TD_Board* current_board = td_current_board(history);
    if (current_board->status != STATUS_RUNNING) {
        return;
    }
    history->tick++;

    if (history->tick == history->count) {
        TD_Timewarps timewarps = {0};
        _td_collect_timewarps(current_board, &timewarps);
        if (timewarps.count > 0) {
            int result_dt = 0;
            for (size_t i = 0; i < timewarps.count; ++i) {
                TD_Timewarp tw = timewarps.items[i];
                if (tw.dt < 1 || (result_dt > 0 && tw.dt != result_dt)) {
                    td_crash(history);
                    return;
                } else if (result_dt == 0) {
                    result_dt = tw.dt;
                }

                for (size_t j = i + 1; j < timewarps.count; ++j) {
                    TD_Timewarp two = timewarps.items[j];
                    if (tw.row == two.row && tw.col == two.col && tw.value != two.value) {
                        td_crash(history);
                        return;
                    }
                }
            }

            size_t tw_time = current_board->time - result_dt;
            int tw_index;
            for (tw_index = history->count - 1; tw_index >= 0; --tw_index) {
                if (history->items[tw_index].time == tw_time) {
                    break;
                }
            }

            if (tw_index < 0) {
                td_crash(history);
                return;
            }

            TD_Board* next_board = td_clone_board(history, tw_index, tw_time);
            for (size_t i = 0; i < timewarps.count; ++i) {
                TD_Timewarp tw = timewarps.items[i];
                td_set_cell(next_board, tw.col, tw.row, make_number_cell(tw.value));
            }

            return;
        }

        TD_Board* next_board = td_clone_board(history, history->count - 1, current_board->time + 1);
        TD_Cell *op_left,  *op_right;
        for (int col = 0; col < (int)history->cols; ++col) {
            for (int row = 0; row < (int)history->rows; ++row) {
                TD_Cell* current_cell = td_cell_at(current_board, col, row);
                switch (current_cell->kind) {
                case CELL_CALC_ADD: {
                    if (td_retrieve_operands(current_board, col, row, &op_left, &op_right)) {
                        td_calculate(next_board, col, row, op_left->value + op_right->value);
                    }
                    break;
                }
                case CELL_CALC_SUBTRACT: {
                    if (td_retrieve_operands(current_board, col, row, &op_left, &op_right)) {
                        td_calculate(next_board, col, row, op_left->value - op_right->value);
                    }
                    break;
                }
                case CELL_CALC_MULTIPLY: {
                    if (td_retrieve_operands(current_board, col, row, &op_left, &op_right)) {
                        td_calculate(next_board, col, row, op_left->value * op_right->value);
                    }
                    break;
                }
                case CELL_CALC_DIVIDE: {
                    if (td_retrieve_operands(current_board, col, row, &op_left, &op_right)) {
                        td_calculate(next_board, col, row, op_left->value / op_right->value);
                    }
                    break;
                }
                case CELL_CALC_REMAINDER: {
                    if (td_retrieve_operands(current_board, col, row, &op_left, &op_right)) {
                        td_calculate(next_board, col, row, op_left->value % op_right->value);
                    }
                    break;
                }
                case CELL_MOVE_LEFT: {
                    TD_Cell* operand = td_cell_at(current_board, col + 1, row );
                    if (operand->kind != CELL_EMPTY) {
                        td_move_left(next_board, col, row, operand);
                    }
                    break;
                }
                case CELL_MOVE_RIGHT: {
                    TD_Cell* operand = td_cell_at(current_board, col - 1, row );
                    if (operand->kind != CELL_EMPTY) {
                        td_move_right(next_board, col, row, operand);
                    }
                    break;
                }
                case CELL_MOVE_UP: {
                    TD_Cell* operand = td_cell_at(current_board, col, row + 1);
                    if (operand->kind != CELL_EMPTY) {
                        td_move_up(next_board, col, row, operand);
                    }
                    break;
                }
                case CELL_MOVE_DOWN: {
                    TD_Cell* operand = td_cell_at(current_board, col, row - 1);
                    if (operand->kind != CELL_EMPTY) {
                        td_move_down(next_board, col, row, operand);
                    }
                    break;
                }
                case CELL_CMP_EQUAL: {
                    if (td_retrieve_operands(current_board, col, row, &op_left, &op_right)) {
                        if (op_left->value == op_right->value) {
                            td_move_right(next_board, col, row, op_left);
                            td_move_down(next_board, col, row, op_right);
                        }
                    }
                    break;
                }
                case CELL_CMP_NOTEQUAL: {
                    if (td_retrieve_operands(current_board, col, row, &op_left, &op_right)) {
                        if (op_left->value != op_right->value) {
                            td_move_right(next_board, col, row, op_left);
                            td_move_down(next_board, col, row, op_right);
                        }
                    }
                    break;
                }
                case CELL_TIMEWARP:
                case CELL_EMPTY:
                case CELL_NUMBER:
                case CELL_STOP:
                    break;

                default:
                    printf("Don't know what to do with cell of kind `%s`.\n", cell_kind_name(current_cell->kind));
                }
            }
        }

        bool changed = false;
        for (int col = 0; col < (int)history->cols; ++col) {
            for (int row = 0; row < (int)history->rows; ++row) {
                TD_Cell* current_cell = td_cell_at(next_board, col, row);
                if (current_cell->active) {
                    changed = true;
                    break;
                }
            }
            if  (changed) {
                break;
            }
        }

        if (!changed) {
            next_board->status = STATUS_STALLED;
        }
    }
}

void td_back(TD_BoardHistory* history) {
    if (history->tick > 0) {
        history->tick--;
    }
}

void td_fast_forward(TD_BoardHistory* history) {
    while (td_current_board(history)->status == STATUS_RUNNING) {
        td_forward(history);
    }
}

void td_rewind(TD_BoardHistory* history) {
    history->tick = 0;
}

void td_reset(TD_BoardHistory* history, int input_a, int input_b) {
    history->count = 1;
    history->tick = 0;

    TD_Board* board = td_current_board(history);
    for (size_t x = 0; x < history->cols; ++x) {
        for (size_t y = 0; y < history->cols; ++y) {
            TD_Cell* cell = td_cell_at(board, x, y);
            if (cell->input_kind == CELL_INPUT_A) {
                cell->value = input_a;
            } else if (cell->input_kind == CELL_INPUT_B) {
                cell->value = input_b;
            }
        }
    }
}
#define INPUT_CELL_COLOR   CLITERAL(Color){ 200, 255, 220, 255 }
#define ACTIVE_CELL_COLOR  CLITERAL(Color){ 255, 255, 220, 255 }
#define STOP_CELL_COLOR    CLITERAL(Color){ 255, 220, 220, 255 }

typedef enum {
    LAYOUT_SCREEN,
    LAYOUT_ANCHOR,
    LAYOUT_STACK,
    LAYOUT_SPACED,
} DW_RLLayoutKind;

typedef enum {
    ANCHOR_TOP,
    ANCHOR_BOTTOM,
    ANCHOR_LEFT,
    ANCHOR_RIGHT,
} DW_RLLayoutAnchorKind;

typedef enum {
    DIRECTION_VERTICAL,
    DIRECTION_HORIZONTAL,
} DW_RLLayoutDirection;

#define WHICH_DEFAULT 0
#define WHICH_UNANCHORED 1
#define WHICH_ANCHORED 2

typedef struct {
    int padding;
} DW_RLLayoutPaddingData;

typedef struct {
    DW_RLLayoutAnchorKind kind;
    int size;
    int gap;
} DW_RLLayoutAnchorData;

typedef struct {
    DW_RLLayoutDirection direction;
    int item_size;
    int gap;
    int* cursor;
} DW_RLLayoutStackData;

typedef struct {
    DW_RLLayoutDirection direction;
    int count;
    int* cursor;
} DW_RLLayoutSpacedData;

typedef union {
    DW_RLLayoutPaddingData screen;
    DW_RLLayoutAnchorData anchored;
    DW_RLLayoutStackData stack;
    DW_RLLayoutSpacedData spaced;
} DW_RLLayoutAs;

typedef struct {
    DW_RLLayoutKind kind;
    Rectangle parent_bounds;
    DW_RLLayoutAs as;
} DW_RLLayout;

typedef struct {
    DW_RLLayout* items;
    size_t capacity;
    size_t count;
} DW_RLLayoutStack;

static DW_RLLayoutStack rl_layout_stack = {0};

Rectangle rl_padding(Rectangle bounds, int padding) {
    Rectangle result = bounds;
    result.x += padding;
    result.y += padding;
    result.width -= 2 * padding;
    result.height -= 2 * padding;
    return result;
}

Rectangle rl_rectangle(int which) {
    NOB_ASSERT(rl_layout_stack.count > 0);

    DW_RLLayout layout = rl_layout_stack.items[rl_layout_stack.count - 1];
    Rectangle result = layout.parent_bounds;

    switch (layout.kind) {
    case LAYOUT_SCREEN:
        result = CLITERAL(Rectangle) {
            .x = 0,
            .y = 0,
            .width = GetScreenWidth(),
            .height = GetScreenHeight(),
        };
        result = rl_padding(result, layout.as.screen.padding);
        break;
    case LAYOUT_ANCHOR: {
        size_t size = layout.as.anchored.size;
        if (which == WHICH_ANCHORED) {
            switch (layout.as.anchored.kind) {
            case ANCHOR_TOP:
                result.height = size;
                break;
            case ANCHOR_LEFT:
                result.height = size;
                break;
            case ANCHOR_BOTTOM:
                result.y += result.height - size;
                result.height = size;
                break;
            case ANCHOR_RIGHT:
                result.x += result.width - size;
                result.width = size;
                break;
            default:
                DW_UNIMPLEMENTED_MSG("Unknown anchor kind `%d`.", layout.as.anchored.kind);
            }
        } else if (which == WHICH_UNANCHORED) {
            int gap = layout.as.anchored.gap;
            switch (layout.as.anchored.kind) {
            case ANCHOR_TOP:
                result.y += size + gap;
                result.height -= size + gap;
                break;
            case ANCHOR_LEFT:
                result.x += size + gap;
                result.width -= size + gap;
                break;
            case ANCHOR_BOTTOM:
                result.height -= size + gap;
                break;
            case ANCHOR_RIGHT:
                result.width -= size + gap;
                break;
            default:
                DW_UNIMPLEMENTED_MSG("Unknown anchor kind `%d`.", layout.as.anchored.kind);
            }
        } else {
            DW_UNIMPLEMENTED_MSG("Unknown anchor which `%d`.", which);
        }
        break;
    }
    case LAYOUT_STACK: {
        int item_size = layout.as.stack.item_size;
        int *cursor = layout.as.stack.cursor;

        switch (layout.as.stack.direction) {
        case DIRECTION_HORIZONTAL:
            result.x += *cursor;
            result.width = item_size;
            break;
        case DIRECTION_VERTICAL:
            result.y += *cursor;
            result.height = item_size;
            break;
        default:
            DW_UNIMPLEMENTED_MSG("Unknown stack direction `%d`.", layout.as.stack.direction);
        }

        *cursor += item_size;
        break;
    }
    case LAYOUT_SPACED: {
        int* cursor = layout.as.spaced.cursor;
        float fract = 1.0 / layout.as.spaced.count;
        float begin = *cursor * fract;
        float size = which * fract;

        switch (layout.as.spaced.direction) {
        case DIRECTION_HORIZONTAL: {
            size_t s = result.width;
            result.x = result.x + s * begin;
            result.width = s * size;
            break;
        }
        case DIRECTION_VERTICAL: {
            size_t s = result.height;
            result.y = result.y + s * begin;
            result.height = s * size;
            break;
        }
        default:
            DW_UNIMPLEMENTED_MSG("Unknown spaced direction `%d`.", layout.as.stack.direction);
        }

        *cursor += which;
        break;
    }

    default:
        DW_UNIMPLEMENTED_MSG("Unknown layout kind `%d`.", layout.kind);
    }

    return  result;
}

void rl_begin_screen(int padding) {
    DW_RLLayout layout = CLITERAL(DW_RLLayout) {
        .kind = LAYOUT_SCREEN,
        .parent_bounds = {0},
        .as.screen.padding = padding,
    };
    nob_da_append(&rl_layout_stack, layout);
}

void rl_begin_anchor(int which, DW_RLLayoutAnchorKind kind, int size, int gap) {
    DW_RLLayout layout = CLITERAL(DW_RLLayout) {
        .kind = LAYOUT_ANCHOR,
        .parent_bounds = rl_rectangle(which),
        .as.anchored = {
            .kind = kind,
            .size = size,
            .gap = gap,
        },
    };
    nob_da_append(&rl_layout_stack, layout);
}

void rl_begin_stack(int which, DW_RLLayoutDirection direction, int item_size, int gap, int* cursor) {
    DW_RLLayout layout = CLITERAL(DW_RLLayout) {
        .kind = LAYOUT_STACK,
        .parent_bounds = rl_rectangle(which),
        .as.stack = {
            .direction = direction,
            .item_size = item_size,
            .gap = gap,
            .cursor = cursor,
        },
    };
    nob_da_append(&rl_layout_stack, layout);
    *layout.as.stack.cursor = 0;
}

void rl_begin_spaced(int which, DW_RLLayoutDirection direction, int count, int* cursor) {
    DW_RLLayout layout = CLITERAL(DW_RLLayout) {
        .kind = LAYOUT_SPACED,
        .parent_bounds = rl_rectangle(which),
        .as.spaced = {
            .direction = direction,
            .count = count,
            .cursor = cursor,
        },
    };
    nob_da_append(&rl_layout_stack, layout);
    *layout.as.spaced.cursor = 0;
}

void rl_spacing(int amount) {
    DW_RLLayout layout = rl_layout_stack.items[rl_layout_stack.count - 1];
    if (layout.kind == LAYOUT_STACK) {
        *layout.as.stack.cursor += amount;
    }
}

void rl_end() {
    NOB_ASSERT(rl_layout_stack.count > 0);
    rl_layout_stack.count--;
}

typedef enum {
    UI_NONE,
    UI_FILENAME,
    UI_INPUT_A,
    UI_INPUT_B,
} UI_Element;

int main(int argc, char** argv)
{
    static const char* symbols[] = { ".", "N", "<", ">", "^", "v", "+", "-", "/", "*", "%", "=", "#", "@", "S" };

    const char* program = nob_shift_args(&argc, &argv);
    if (argc < 1) {
        nob_log(NOB_ERROR, "Usage: %s filename [input a] [input b]", program);
        exit(1);
    }

    const char* filename = nob_shift_args(&argc, &argv);

    int input_a = 0;
    if (argc > 0) {
        input_a = atoi(nob_shift_args(&argc, &argv));
    }

    int input_b = 0;
    if (argc > 0) {
        input_b = atoi(nob_shift_args(&argc, &argv));
    }


    TD_BoardHistory history = {0};
    td_read(&history, filename, input_a, input_b);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);

    InitWindow(16*90, 9*90, "d3IDE");

    SetTargetFPS(60);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

    int gui_input_a = history.input_a;
    int gui_input_b = history.input_b;
    char gui_filename[1024];
    strncpy(gui_filename, filename, 1024);

    static UI_Element gui_active_element = UI_NONE;

    while (!WindowShouldClose()) {
        TD_Board* current_board = td_current_board(&history);

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        rl_begin_screen(10);
        {
            rl_begin_anchor(WHICH_DEFAULT, ANCHOR_TOP, 30, 10);
            {
                rl_begin_anchor(WHICH_ANCHORED, ANCHOR_RIGHT, 30, 10);
                {
                    if (GuiTextBox(rl_rectangle(WHICH_UNANCHORED), gui_filename, 1024, gui_active_element == UI_FILENAME)) {
                        if (gui_active_element == UI_FILENAME) {
                            gui_active_element = UI_NONE;
                            td_read(&history, gui_filename, gui_input_a, gui_input_b);
                        } else {
                            gui_active_element = UI_FILENAME;
                        }
                    }

                    if (GuiButton(rl_rectangle(WHICH_ANCHORED), "#75#") || IsKeyPressed(KEY_F5)) {
                        td_read(&history, gui_filename, gui_input_a, gui_input_b);
                    }
                }
                rl_end();

                rl_begin_anchor(WHICH_UNANCHORED, ANCHOR_RIGHT, 200, 10);
                {
                    // GRID
                    {
                        Rectangle grid_bounds = rl_rectangle(WHICH_UNANCHORED);
                        int cell_size = min(grid_bounds.width / history.cols, grid_bounds.height / history.rows);

                        for (size_t x = 0; x < history.cols; ++x) {
                            for (size_t y = 0; y < history.rows; ++y) {
                                size_t ox = grid_bounds.x + x * cell_size;
                                size_t oy = grid_bounds.y + y * cell_size;

                                TD_Cell* cell = td_cell_at(current_board, x, y);


                                if (cell->active) {
                                    DrawRectangle(ox, oy, cell_size, cell_size, ACTIVE_CELL_COLOR);
                                } else if (cell->input_kind == CELL_INPUT_A || cell->input_kind == CELL_INPUT_B) {
                                    DrawRectangle(ox, oy, cell_size, cell_size, INPUT_CELL_COLOR);
                                    if (cell->input_kind == CELL_INPUT_A) {
                                        DrawText("A", ox + 4, oy + 2, cell_size / 8, BROWN);
                                    } else if (cell->input_kind == CELL_INPUT_B) {
                                        DrawText("B", ox + 4, oy + 2, cell_size / 8, BROWN);
                                    }
                                } else if (cell->kind == CELL_STOP) {
                                    DrawRectangle(ox, oy, cell_size, cell_size, STOP_CELL_COLOR);
                                    DrawText("S", ox + 4, oy + 2, cell_size / 8, RED);
                                }

                                Color text_color = GRAY;

                                switch (cell->kind) {
                                case CELL_EMPTY:
                                case CELL_STOP:
                                    DrawCircle(ox + cell_size / 2, oy + cell_size / 2, cell_size / 16, LIGHTGRAY);
                                    break;
                                case     CELL_NUMBER: {
                                    const char* text = TextFormat("%d", cell->value);
                                    int text_height = cell_size * 1 / 2;
                                    int text_width = MeasureText(text, text_height);
                                    DrawText(text, ox + cell_size / 2 - text_width / 2, oy + cell_size / 2 - text_height / 2, text_height, text_color);
                                    break;
                                }
                                case CELL_MOVE_LEFT:
                                case CELL_MOVE_RIGHT:
                                case CELL_MOVE_UP:
                                case CELL_MOVE_DOWN:
                                case CELL_CALC_ADD:
                                case CELL_CALC_SUBTRACT:
                                case CELL_CALC_DIVIDE:
                                case CELL_CALC_MULTIPLY:
                                case CELL_CALC_REMAINDER:
                                case CELL_CMP_EQUAL:
                                case CELL_CMP_NOTEQUAL:
                                case CELL_TIMEWARP: {
                                    const char* text = symbols[cell->kind];
                                    int text_height = cell_size * 1 / 2;
                                    int text_width = MeasureText(text, text_height);
                                    DrawText(text, ox + cell_size / 2 - text_width / 2, oy + cell_size / 2 - text_height / 2, text_height, text_color);
                                    break;
                                }
                                default: {
                                    DrawRectangle(ox, oy, cell_size, cell_size, RED);
                                    DrawText(cell_kind_name(cell->kind), ox + 4, oy + 4, 16, GRAY);
                                    break;
                                }
                                }
                            }

                        }

                        for (size_t x = 0; x <= history.cols; ++x) {
                            DrawLine(grid_bounds.x + x * cell_size,
                                     grid_bounds.y,
                                     grid_bounds.x + x * cell_size,
                                     grid_bounds.y + history.rows * cell_size,
                                     LIGHTGRAY);
                        }
                        for (size_t y = 0; y <= history.rows; ++y) {
                            DrawLine(grid_bounds.x,
                                     grid_bounds.y + y * cell_size,
                                     grid_bounds.x + history.cols * cell_size,
                                     grid_bounds.y + y * cell_size,
                                     LIGHTGRAY);
                        }
                    }

                    int panel_cursor;
                    rl_begin_stack(WHICH_ANCHORED, DIRECTION_VERTICAL, 30, 0, &panel_cursor);
                    {
                        GuiLabel(rl_rectangle(WHICH_DEFAULT), status_name(current_board->status));
                        GuiLabel(rl_rectangle(WHICH_DEFAULT), TextFormat("Tick %zu/%zu", history.tick + 1, history.count));
                        GuiLabel(rl_rectangle(WHICH_DEFAULT), TextFormat("Time %zd", current_board->time));

                        rl_spacing(8);
                        int panel_nav_cursor;
                        rl_begin_spaced(WHICH_DEFAULT, DIRECTION_HORIZONTAL, 6, &panel_nav_cursor);
                        {
                            GuiSetState(history.tick > 0 ? STATE_NORMAL : STATE_DISABLED);
                            if (GuiButton(rl_rectangle(1), "<<") || IsKeyPressed(KEY_HOME)) {
                                td_rewind(&history);
                            }
                            if (GuiButton(rl_rectangle(2), "<") || IsKeyPressed(KEY_LEFT)) {
                                td_back(&history);
                            }

                            GuiSetState(current_board->status == STATUS_RUNNING ? STATE_NORMAL : STATE_DISABLED);
                            if (GuiButton(rl_rectangle(2), ">") || IsKeyPressed(KEY_RIGHT)) {
                                td_forward(&history);
                            }
                            if (GuiButton(rl_rectangle(1), ">>") || IsKeyPressed(KEY_END)) {
                                td_fast_forward(&history);
                            }
                            GuiEnable();
                        }
                        rl_end();

                        rl_spacing(8);
                        GuiLabel(rl_rectangle(WHICH_DEFAULT), "Input A");
                        GuiSpinner(rl_rectangle(WHICH_DEFAULT), NULL, &gui_input_a, INT_MIN, INT_MAX, false);

                        rl_spacing(8);
                        GuiLabel(rl_rectangle(WHICH_DEFAULT), "Input B");
                        GuiSpinner(rl_rectangle(WHICH_DEFAULT), NULL, &gui_input_b, INT_MIN, INT_MAX, false);

                        rl_spacing(8);
                        if (GuiButton(rl_rectangle(WHICH_DEFAULT), "Reset") || IsKeyPressed(KEY_R)) {
                            td_reset(&history, gui_input_a, gui_input_b);
                        }

                        if (current_board->status == STATUS_STOPPED) {
                            rl_spacing(8);
                            GuiLabel(rl_rectangle(WHICH_DEFAULT), TextFormat("Result: %d", current_board->result));
                        }
                    }
                    rl_end();
                }
                rl_end();
            }
            rl_end();
        }
        rl_end();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}