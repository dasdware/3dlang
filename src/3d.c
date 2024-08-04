#include <stdio.h>
#include <raylib.h>

#include <error.h>
#include <3dl.h>

#define RL_DIALOG_IMPLEMENTATION
#include <rldialog.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#define RLLAYOUT_IMPLEMENTATION
#include <rllayout.h>

#define ARENA_IMPLEMENTATION
#include <arena.h>

#define NOB_IMPLEMENTATION
#include <nob.h>

#define INPUT_CELL_COLOR   CLITERAL(Color){ 200, 255, 220, 255 }
#define ACTIVE_CELL_COLOR  CLITERAL(Color){ 255, 255, 220, 255 }
#define STOP_CELL_COLOR    CLITERAL(Color){ 255, 220, 220, 255 }

typedef enum {
    UI_NONE,
    UI_FILENAME,
    UI_INPUT_A,
    UI_INPUT_B,
} UI_Element;

typedef struct {
    TD_BoardHistory history;
    RL_FileDialog open_dialog;
    int gui_input_a;
    int gui_input_b;
    char gui_filename[1024];
} UI_State;

void run_screen(UI_State *state) {
    static const char* symbols[] = { ".", "N", "<", ">", "^", "v", "+", "-", "/", "*", "%", "=", "#", "@", "S" };
    static UI_Element gui_active_element = UI_NONE;

    TD_Board* current_board = td_current_board(&state->history);

    LayoutBeginScreen(10);
    {
        LayoutBeginAnchored(RLD_DEFAULT, 10);
        {
            LayoutBeginStack(RL_ANCHOR_TOP(30), DIRECTION_HORIZONTAL, 30, 10);
            {
                if (GuiButton(LayoutOpposite(), "#75#") || (!guiLocked && IsKeyPressed(KEY_F5))) {
                    td_read(&state->history, state->gui_filename, state->gui_input_a, state->gui_input_b);
                }

                if (GuiButton(LayoutOpposite(), "#01#") || (!guiLocked && IsKeyPressed(KEY_O))) {
                    GuiFileDialogOpen(&state->open_dialog);
                }

                if (GuiTextBox(LayoutRemaining(), state->gui_filename, 1024, gui_active_element == UI_FILENAME)) {
                    if (gui_active_element == UI_FILENAME) {
                        gui_active_element = UI_NONE;
                        td_read(&state->history, state->gui_filename, state->gui_input_a, state->gui_input_b);
                    } else {
                        gui_active_element = UI_FILENAME;
                    }
                }
            }
            LayoutEnd();

            LayoutBeginAnchored(RLD_REMAINING, 10);
            {
                LayoutBeginStack(RL_ANCHOR_RIGHT(200), DIRECTION_VERTICAL, 30, 0);
                {
                    GuiLabel(LayoutDefault(), td_status_name(current_board->status));
                    GuiLabel(LayoutDefault(), TextFormat("Tick %zu/%zu", state->history.tick + 1, state->history.count));
                    GuiLabel(LayoutDefault(), TextFormat("Time %zd", current_board->time));

                    LayoutSpacing(8);

                    LayoutBeginSpaced(RLD_DEFAULT, DIRECTION_HORIZONTAL, 6, 5);
                    {
                        GuiSetState(state->history.tick > 0 ? STATE_NORMAL : STATE_DISABLED);
                        if (GuiButton(LayoutRectangle(1), "<<") || (!guiLocked && IsKeyPressed(KEY_HOME))) {
                            td_rewind(&state->history);
                        }
                        if (GuiButton(LayoutRectangle(2), "<") || (!guiLocked && IsKeyPressed(KEY_LEFT))) {
                            td_back(&state->history);
                        }

                        GuiSetState(current_board->status == STATUS_RUNNING ? STATE_NORMAL : STATE_DISABLED);
                        if (GuiButton(LayoutRectangle(2), ">") || (!guiLocked && IsKeyPressed(KEY_RIGHT))) {
                            td_forward(&state->history);
                        }
                        if (GuiButton(LayoutRectangle(1), ">>") || (!guiLocked && IsKeyPressed(KEY_END))) {
                            td_fast_forward(&state->history);
                        }
                        GuiEnable();
                    }
                    LayoutEnd();

                    LayoutSpacing(8);
                    GuiLabel(LayoutDefault(), "Input A");
                    GuiSpinner(LayoutDefault(), NULL, &state->gui_input_a, INT_MIN, INT_MAX, false);

                    LayoutSpacing(8);
                    GuiLabel(LayoutDefault(), "Input B");
                    GuiSpinner(LayoutDefault(), NULL, &state->gui_input_b, INT_MIN, INT_MAX, false);

                    LayoutSpacing(8);
                    if (GuiButton(LayoutDefault(), "Reset") || (!guiLocked && IsKeyPressed(KEY_R))) {
                        td_reset(&state->history, state->gui_input_a, state->gui_input_b);
                    }

                    if (current_board->status == STATUS_STOPPED) {
                        LayoutSpacing(8);
                        GuiLabel(LayoutDefault(), TextFormat("Result: %d", current_board->result));
                    }
                }
                LayoutEnd();

                // GRID
                {
                    Rectangle grid_bounds = LayoutDefault();
                    int cell_size = min(grid_bounds.width / state->history.cols, grid_bounds.height / state->history.rows);

                    TD_FOREACH(current_board, cursor) {
                        size_t ox = grid_bounds.x + cursor.col * cell_size;
                        size_t oy = grid_bounds.y + cursor.row * cell_size;

                        if (cursor.cell->active) {
                            DrawRectangle(ox, oy, cell_size, cell_size, ACTIVE_CELL_COLOR);
                        } else if (cursor.cell->input_kind == CELL_INPUT_A || cursor.cell->input_kind == CELL_INPUT_B) {
                            DrawRectangle(ox, oy, cell_size, cell_size, INPUT_CELL_COLOR);
                            if (cursor.cell->input_kind == CELL_INPUT_A) {
                                DrawText("A", ox + 4, oy + 2, cell_size / 8, BROWN);
                            } else if (cursor.cell->input_kind == CELL_INPUT_B) {
                                DrawText("B", ox + 4, oy + 2, cell_size / 8, BROWN);
                            }
                        } else if (cursor.cell->kind == CELL_STOP) {
                            DrawRectangle(ox, oy, cell_size, cell_size, STOP_CELL_COLOR);
                            DrawText("S", ox + 4, oy + 2, cell_size / 8, RED);
                        }

                        Color text_color = GRAY;

                        switch (cursor.cell->kind) {
                        case CELL_EMPTY:
                        case CELL_STOP:
                            DrawCircle(ox + cell_size / 2, oy + cell_size / 2, cell_size / 16, LIGHTGRAY);
                            break;
                        case     CELL_NUMBER: {
                            const char* text = TextFormat("%d", cursor.cell->value);
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
                            const char* text = symbols[cursor.cell->kind];
                            int text_height = cell_size * 1 / 2;
                            int text_width = MeasureText(text, text_height);
                            DrawText(text, ox + cell_size / 2 - text_width / 2, oy + cell_size / 2 - text_height / 2, text_height, text_color);
                            break;
                        }
                        default: {
                            DrawRectangle(ox, oy, cell_size, cell_size, RED);
                            DrawText(td_cell_kind_name(cursor.cell->kind), ox + 4, oy + 4, 16, GRAY);
                            break;
                        }
                        }
                    }

                    for (size_t x = 0; x <= state->history.cols; ++x) {
                        DrawLine(grid_bounds.x + x * cell_size,
                                 grid_bounds.y,
                                 grid_bounds.x + x * cell_size,
                                 grid_bounds.y + state->history.rows * cell_size,
                                 LIGHTGRAY);
                    }
                    for (size_t y = 0; y <= state->history.rows; ++y) {
                        DrawLine(grid_bounds.x,
                                 grid_bounds.y + y * cell_size,
                                 grid_bounds.x + state->history.cols * cell_size,
                                 grid_bounds.y + y * cell_size,
                                 LIGHTGRAY);
                    }
                }
            }
            LayoutEnd();
        }
        LayoutEnd();
    }
    LayoutEnd();
}

int main(int argc, char** argv)
{
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

    UI_State state = {0};
    td_read(&state.history, filename, input_a, input_b);
    state.gui_input_a = state.history.input_a;
    state.gui_input_b = state.history.input_b;
    strncpy(state.gui_filename, filename, 1024);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(16*90, 9*90, "3dIDE");
    SetTargetFPS(60);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    SetExitKey(0);

    GuiFileDialogInit(&state.open_dialog, "Open 3dl Program", filename, ".3dl");

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        GuiFileDialogCheck(&state.open_dialog);

        run_screen(&state);

        if (GuiFileDialog(&state.open_dialog)) {
            const char* filename = GuiFileDialogFileName(&state.open_dialog);
            td_read(&state.history, filename, state.gui_input_a, state.gui_input_b);
            strncpy(state.gui_filename, filename, 1024);
        }

        EndDrawing();
    }

    CloseWindow();
    GuiFileDialogFree(&state.open_dialog);

    return 0;
}