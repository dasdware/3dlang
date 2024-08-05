#include <3dl.h>
#include <nob.h>

// Enum operations

const char* td_cell_kind_name(TD_CellKind kind) {
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

const char* td_status_name(TD_Status status) {
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

// Loading / Freeing

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

    history->loaded = true;
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

// Cell operations

TD_Cell _td_make_empty_cell() {
    return (TD_Cell) {
        0
    };
}

TD_Cell _td_make_number_cell(int value) {
    return (TD_Cell) {
        .kind = CELL_NUMBER,
        .value = value,
    };
}

void _td_set_cell(TD_BoardCursor cursor, TD_Cell value) {
    TD_CellInputKind old_input_kind = cursor.cell->input_kind;
    bool stopped = cursor.cell->kind == CELL_STOP;

    *cursor.cell = value;
    cursor.cell->input_kind = old_input_kind;
    if (stopped) {
        cursor.board->status = STATUS_STOPPED;
        cursor.board->result = value.value;
    }
}

void _td_activate_cell(TD_BoardCursor cursor) {
    cursor.cell->active = true;
}

// History navigation

TD_Board* td_current_board(TD_BoardHistory* history) {
    return &history->items[history->tick];
}

bool _td_retrieve_operands(TD_BoardCursor cursor,
                           TD_Cell** left, TD_Cell** right) {
    *left = td_cursor_left(cursor).cell;
    *right = td_cursor_up(cursor).cell;
    return (*left)->kind == CELL_NUMBER
           && (*right)->kind == CELL_NUMBER;
}

bool _td_retrieve_timewarp_operands(TD_BoardCursor cursor,
                                    TD_Cell** op_v, TD_Cell** op_dx, TD_Cell** op_dy, TD_Cell** op_dt) {
    *op_v = td_cursor_up(cursor).cell;
    *op_dx = td_cursor_left(cursor).cell;
    *op_dy = td_cursor_right(cursor).cell;
    *op_dt = td_cursor_down(cursor).cell;
    return (*op_v)->kind == CELL_NUMBER
           && (*op_dx)->kind == CELL_NUMBER
           && (*op_dy)->kind == CELL_NUMBER
           && (*op_dt)->kind == CELL_NUMBER;
}

void _td_calculate(TD_BoardCursor cursor, int value) {
    _td_set_cell(td_cursor_left(cursor), _td_make_empty_cell());
    _td_set_cell(td_cursor_up(cursor), _td_make_empty_cell());
    _td_set_cell(td_cursor_right(cursor), _td_make_number_cell(value));
    _td_set_cell(td_cursor_down(cursor), _td_make_number_cell(value));
    _td_activate_cell(cursor);
    _td_activate_cell(td_cursor_right(cursor));
    _td_activate_cell(td_cursor_down(cursor));
}

void _td_move_left(TD_BoardCursor cursor, TD_Cell* operand) {
    _td_set_cell(td_cursor_right(cursor), _td_make_empty_cell());
    _td_set_cell(td_cursor_left(cursor), *operand);
    _td_activate_cell(cursor);
    _td_activate_cell(td_cursor_left(cursor));
}

void _td_move_right(TD_BoardCursor cursor, TD_Cell* operand) {
    _td_set_cell(td_cursor_left(cursor), _td_make_empty_cell());
    _td_set_cell(td_cursor_right(cursor), *operand);
    _td_activate_cell(cursor);
    _td_activate_cell(td_cursor_right(cursor));
}

void _td_move_up(TD_BoardCursor cursor, TD_Cell* operand) {
    _td_set_cell(td_cursor_down(cursor), _td_make_empty_cell());
    _td_set_cell(td_cursor_up(cursor), *operand);
    _td_activate_cell(cursor);
    _td_activate_cell(td_cursor_up(cursor));
}

void _td_move_down(TD_BoardCursor cursor, TD_Cell* operand) {
    _td_set_cell(td_cursor_up(cursor), _td_make_empty_cell());
    _td_set_cell(td_cursor_down(cursor), *operand);
    _td_activate_cell(cursor);
    _td_activate_cell(td_cursor_down(cursor));
}

void _td_collect_timewarps(TD_Board* board, TD_Timewarps* timewarps) {
    TD_Cell *op_v, *op_dx, *op_dy, *op_dt;
    TD_FOREACH(board, cursor) {
        switch (cursor.cell->kind) {
        case CELL_TIMEWARP: {
            if (_td_retrieve_timewarp_operands(cursor, &op_v, &op_dx, &op_dy, &op_dt)) {
                TD_Timewarp tw = {
                    .timewarp_cursor = cursor,
                    .cell_cursor = td_cursor_move(cursor, -op_dx->value, -op_dy->value),
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

TD_Board* _td_clone_board(TD_BoardHistory *history, size_t index, int time) {
    TD_Board* board = &history->items[index];

    TD_Board new_board = {0};
    new_board.history = history;
    new_board.result = 0;
    new_board.status = STATUS_RUNNING;
    new_board.time = time;
    new_board.cells = arena_alloc(&history->cells_arena, history->cells_bytes);
    memcpy(new_board.cells, board->cells, history->cells_bytes);

    TD_FOREACH(&new_board, cursor) {
        cursor.cell->active = false;
    }

    nob_da_append(history, new_board);

    return &history->items[history->count - 1];
}

void _td_crash(TD_BoardHistory* history) {
    TD_Board* current_board = td_current_board(history);
    TD_Board* next_board = _td_clone_board(history, history->count - 1, current_board->time + 1);
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
                    _td_crash(history);
                    return;
                } else if (result_dt == 0) {
                    result_dt = tw.dt;
                }

                for (size_t j = i + 1; j < timewarps.count; ++j) {
                    TD_Timewarp two = timewarps.items[j];
                    if (td_cursor_same(tw.cell_cursor, two.cell_cursor) && tw.value != two.value) {
                        _td_crash(history);
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
                _td_crash(history);
                return;
            }

            TD_Board* next_board = _td_clone_board(history, tw_index, tw_time);
            for (size_t i = 0; i < timewarps.count; ++i) {
                TD_Timewarp tw = timewarps.items[i];
                _td_set_cell(td_cursor_board(tw.cell_cursor, next_board), _td_make_number_cell(tw.value));
                _td_activate_cell(td_cursor_board(tw.timewarp_cursor, next_board));
                _td_activate_cell(td_cursor_board(tw.cell_cursor, next_board));
            }

            return;
        }

        TD_Board* next_board = _td_clone_board(history, history->count - 1, current_board->time + 1);
        TD_Cell *op_left,  *op_right;
        TD_FOREACH(current_board, current_cursor) {
            TD_BoardCursor next_cursor = td_cursor_board(current_cursor, next_board);
            switch (current_cursor.cell->kind) {
            case CELL_CALC_ADD: {
                if (_td_retrieve_operands(current_cursor, &op_left, &op_right)) {
                    _td_calculate(next_cursor, op_left->value + op_right->value);
                }
                break;
            }
            case CELL_CALC_SUBTRACT: {
                if (_td_retrieve_operands(current_cursor, &op_left, &op_right)) {
                    _td_calculate(next_cursor, op_left->value - op_right->value);
                }
                break;
            }
            case CELL_CALC_MULTIPLY: {
                if (_td_retrieve_operands(current_cursor, &op_left, &op_right)) {
                    _td_calculate(next_cursor, op_left->value * op_right->value);
                }
                break;
            }
            case CELL_CALC_DIVIDE: {
                if (_td_retrieve_operands(current_cursor, &op_left, &op_right)) {
                    _td_calculate(next_cursor, op_left->value / op_right->value);
                }
                break;
            }
            case CELL_CALC_REMAINDER: {
                if (_td_retrieve_operands(current_cursor, &op_left, &op_right)) {
                    _td_calculate(next_cursor, op_left->value % op_right->value);
                }
                break;
            }
            case CELL_MOVE_LEFT: {
                TD_Cell* operand = td_cursor_right(current_cursor).cell;
                if (operand->kind != CELL_EMPTY) {
                    _td_move_left(next_cursor, operand);
                }
                break;
            }
            case CELL_MOVE_RIGHT: {
                TD_Cell* operand = td_cursor_left(current_cursor).cell;
                if (operand->kind != CELL_EMPTY) {
                    _td_move_right(next_cursor, operand);
                }
                break;
            }
            case CELL_MOVE_UP: {
                TD_Cell* operand = td_cursor_down(current_cursor).cell;
                if (operand->kind != CELL_EMPTY) {
                    _td_move_up(next_cursor, operand);
                }
                break;
            }
            case CELL_MOVE_DOWN: {
                TD_Cell* operand = td_cursor_up(current_cursor).cell;
                if (operand->kind != CELL_EMPTY) {
                    _td_move_down(next_cursor, operand);
                }
                break;
            }
            case CELL_CMP_EQUAL: {
                if (_td_retrieve_operands(current_cursor, &op_left, &op_right)) {
                    if (op_left->value == op_right->value) {
                        _td_move_right(next_cursor, op_left);
                        _td_move_down(next_cursor, op_right);
                    }
                }
                break;
            }
            case CELL_CMP_NOTEQUAL: {
                if (_td_retrieve_operands(current_cursor, &op_left, &op_right)) {
                    if (op_left->value != op_right->value) {
                        _td_move_right(next_cursor, op_left);
                        _td_move_down(next_cursor, op_right);
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
                printf("Don't know what to do with cell of kind `%s`.\n", td_cell_kind_name(current_cursor.cell->kind));
            }
        }

        bool changed = false;
        TD_FOREACH(next_board, cursor) {
            if (cursor.cell->active) {
                changed = true;
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
    TD_FOREACH(board, cursor) {
        if (cursor.cell->input_kind == CELL_INPUT_A) {
            cursor.cell->value = input_a;
        } else if (cursor.cell->input_kind == CELL_INPUT_B) {
            cursor.cell->value = input_b;
        }
    }
}

// Cursor operations

TD_BoardCursor _td_cursor_validate(TD_BoardCursor cursor) {
    static TD_Cell empty_cell = {0};

    cursor.valid = (cursor.col >= 0) && (cursor.col < (int)cursor.board->history->cols)
                   && (cursor.row >= 0) && (cursor.row < (int)cursor.board->history->rows);
    if (cursor.valid) {
        cursor.cell = &cursor.board->cells[cursor.row * cursor.board->history->cols + cursor.col];
    } else {
        cursor.cell = &empty_cell;
    }

    return cursor;
}

TD_BoardCursor td_cursor_first(TD_Board* board) {
    TD_BoardCursor cursor = {
        .board = board,
        .col = 0,
        .row = 0,
    };
    return _td_cursor_validate(cursor);
}

TD_BoardCursor td_cursor_next(TD_BoardCursor cursor) {
    cursor.col++;
    if (cursor.col == (int)cursor.board->history->cols) {
        cursor.row++;
        cursor.col = 0;
    }
    return _td_cursor_validate(cursor);
}

TD_BoardCursor td_cursor_board(TD_BoardCursor cursor, TD_Board* board) {
    cursor.board = board;
    return _td_cursor_validate(cursor);
}

TD_BoardCursor td_cursor_move(TD_BoardCursor cursor, int cols, int rows) {
    cursor.col += cols;
    cursor.row += rows;
    return _td_cursor_validate(cursor);
}

bool td_cursor_same(TD_BoardCursor first, TD_BoardCursor second) {
    return (first.col == second.col) && (first.row == second.row);
}