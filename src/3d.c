#include <stdio.h>
#include <raylib.h>

#include <error.h>
#include <3dl.h>


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
                    if (GuiTextBox(rl_rectangle(), gui_filename, 1024, gui_active_element == UI_FILENAME)) {
                        if (gui_active_element == UI_FILENAME) {
                            gui_active_element = UI_NONE;
                            td_read(&history, gui_filename, gui_input_a, gui_input_b);
                        } else {
                            gui_active_element = UI_FILENAME;
                        }
                    }

                    if (GuiButton(rl_rectangle(), "#75#") || IsKeyPressed(KEY_F5)) {
                        td_read(&history, gui_filename, gui_input_a, gui_input_b);
                    }
                }
                rl_end();

                rl_begin_anchor(WHICH_REMAINING, ANCHOR_RIGHT, 200, 10);
                {
                    // GRID
                    {
                        Rectangle grid_bounds = rl_rectangle();
                        int cell_size = min(grid_bounds.width / history.cols, grid_bounds.height / history.rows);

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

                    rl_begin_stack(WHICH_ANCHORED, DIRECTION_VERTICAL, 30, 0);
                    {
                        GuiLabel(rl_rectangle(), td_status_name(current_board->status));
                        GuiLabel(rl_rectangle(), TextFormat("Tick %zu/%zu", history.tick + 1, history.count));
                        GuiLabel(rl_rectangle(), TextFormat("Time %zd", current_board->time));

                        rl_spacing(8);

                        rl_begin_spaced(WHICH_DEFAULT, DIRECTION_HORIZONTAL, 6);
                        {
                            GuiSetState(history.tick > 0 ? STATE_NORMAL : STATE_DISABLED);
                            if (GuiButton(rl_rectangle_which(1), "<<") || (!guiLocked && IsKeyPressed(KEY_HOME))) {
                                td_rewind(&history);
                            }
                            if (GuiButton(rl_rectangle_which(2), "<") || (!guiLocked && IsKeyPressed(KEY_LEFT))) {
                                td_back(&history);
                            }

                            GuiSetState(current_board->status == STATUS_RUNNING ? STATE_NORMAL : STATE_DISABLED);
                            if (GuiButton(rl_rectangle_which(2), ">") || (!guiLocked && IsKeyPressed(KEY_RIGHT))) {
                                td_forward(&history);
                            }
                            if (GuiButton(rl_rectangle_which(1), ">>") || (!guiLocked && IsKeyPressed(KEY_END))) {
                                td_fast_forward(&history);
                            }
                            GuiEnable();
                        }
                        rl_end();

                        rl_spacing(8);
                        GuiLabel(rl_rectangle(), "Input A");
                        GuiSpinner(rl_rectangle(), NULL, &gui_input_a, INT_MIN, INT_MAX, false);

                        rl_spacing(8);
                        GuiLabel(rl_rectangle(), "Input B");
                        GuiSpinner(rl_rectangle(), NULL, &gui_input_b, INT_MIN, INT_MAX, false);

                        rl_spacing(8);
                        if (GuiButton(rl_rectangle(), "Reset") || (!guiLocked && IsKeyPressed(KEY_R))) {
                            td_reset(&history, gui_input_a, gui_input_b);
                        }

                        if (current_board->status == STATUS_STOPPED) {
                            rl_spacing(8);
                            GuiLabel(rl_rectangle(), TextFormat("Result: %d", current_board->result));
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