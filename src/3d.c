#include <stdio.h>
#include <raylib.h>

#include <error.h>
#include <3dl.h>

#define RL_DIALOG_IMPLEMENTATION
#include <rldialog.h>

#define RL_GUIEXT_IMPLEMENTATION
#include <rlguiext.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#define RLLAYOUT_IMPLEMENTATION
#include <rllayout.h>

#define ARENA_IMPLEMENTATION
#include <arena.h>

#define NOB_IMPLEMENTATION
#include <nob.h>

#define DW_ARRAY_IMPLEMENTATION
#include <dw_array.h>

#define PROGRAM_TITLE "3dIDE"

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
    bool close_requested;
    Vector2 grid_scroll;
    int grid_zoom;
} UI_State;

void load_file(UI_State* state, const char* filename)
{
    strncpy(state->gui_filename, filename, 1024);
    td_read(&state->history, filename, state->gui_input_a, state->gui_input_b);
    SetWindowTitle(TextFormat("%s - %s", state->gui_filename, PROGRAM_TITLE));
}

void initial_screen(UI_State *state) {
    LayoutBeginScreen(0);
    {
        LayoutBeginRectangle(RLD_DEFAULT, LayoutCenter(LayoutDefault(), 430, 110));
        {
            LayoutBeginStack(RLD_DEFAULT, DIRECTION_VERTICAL, 30, 10);
            {
                LayoutBeginStack(RLD_DEFAULT, DIRECTION_HORIZONTAL, 30, 10);
                {
                    LayoutGuiLabel("Press");
                    LayoutGuiButtonLabel("N");
                    LayoutGuiLabel("to create a new empty 3D program.");
                }
                LayoutEnd();

                LayoutBeginStack(RLD_DEFAULT, DIRECTION_HORIZONTAL, 30, 10);
                {
                    LayoutGuiLabel("Press");
                    LayoutGuiButtonLabel("O");
                    LayoutGuiLabel("to open an existing 3D program file.");
                }
                LayoutEnd();

                LayoutBeginStack(RLD_DEFAULT, DIRECTION_HORIZONTAL, 30, 10);
                {
                    LayoutGuiLabel("Press");
                    LayoutGuiButtonLabel("ESC");
                    LayoutGuiLabel("to exit the application.");
                }
                LayoutEnd();
            }
            LayoutEnd();
        }
        LayoutEnd();
    }
    LayoutEnd();
    if (GuiIsKeyPressed(KEY_O)) {
        GuiFileDialogOpen(&state->open_dialog);
    } else if (GuiIsKeyPressed(KEY_ESCAPE)) {
        state->close_requested = true;
    }
}

void run_screen(UI_State *state) {
    static const char* symbols[] = { ".", "N", "<", ">", "^", "v", "+", "-", "/", "*", "%", "=", "#", "@", "S" };
    static const int zoom_levels[] = { 25, 50, 75, 100, 150, 200 };
    static const int zoom_level_min = 0;
    static const int zoom_level_max = NOB_ARRAY_LEN(zoom_levels) - 1;
    static const int zoom_level_default = 3;

    TD_Board* current_board = td_current_board(&state->history);

    LayoutBeginScreen(10);
    {
        LayoutBeginAnchored(RLD_DEFAULT, 10);
        {
            LayoutBeginStack(RL_ANCHOR_TOP(30), DIRECTION_HORIZONTAL, 30, 10);
            {
                if (GuiButton(LayoutDefault(), "#159#") || GuiIsKeyPressed(KEY_ESCAPE)) {
                    state->close_requested = true;
                }

                if (GuiButton(LayoutDefault(), "#01#") || GuiIsKeyPressed(KEY_O)) {
                    GuiFileDialogOpen(&state->open_dialog);
                }

                if (GuiButton(LayoutDefault(), "#75#") || GuiIsKeyPressed(KEY_F5)) {
                    td_read(&state->history, state->gui_filename, state->gui_input_a, state->gui_input_b);
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
                        if (GuiButton(LayoutRectangle(1), "<<") || GuiIsKeyPressed(KEY_HOME)) {
                            td_rewind(&state->history);
                        }
                        if (GuiButton(LayoutRectangle(2), "<") || GuiIsKeyPressed(KEY_LEFT)) {
                            td_back(&state->history);
                        }

                        GuiSetState(current_board->status == STATUS_RUNNING ? STATE_NORMAL : STATE_DISABLED);
                        if (GuiButton(LayoutRectangle(2), ">") || GuiIsKeyPressed(KEY_RIGHT)) {
                            td_forward(&state->history);
                        }
                        if (GuiButton(LayoutRectangle(1), ">>") || GuiIsKeyPressed(KEY_END)) {
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
                    if (GuiButton(LayoutDefault(), "Reset") || GuiIsKeyPressed(KEY_R)) {
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
                    LayoutBeginAnchored(RLD_DEFAULT, 5);
                    {
                        LayoutBeginStack(RL_ANCHOR_BOTTOM(29), DIRECTION_HORIZONTAL, 29, 10);
                        {
                            if (state->grid_zoom == zoom_level_default) {
                                GuiDisable();
                            }
                            if (GuiButton(LayoutOpposite(), "1:1") || GuiIsKeyPressed(KEY_KP_0)) {
                                state->grid_zoom = zoom_level_default;
                            }
                            GuiEnable();

                            if (state->grid_zoom >= zoom_level_max) {
                                GuiDisable();
                            }
                            if ((GuiButton(LayoutOpposite(), "+") || GuiIsKeyPressed(KEY_KP_ADD)) && state->grid_zoom < zoom_level_max) {
                                state->grid_zoom++;
                            }
                            GuiEnable();

                            Rectangle zoom_text_bounds = LayoutRectangle(RL_OPPOSITE(GetTextWidth("100%") + 2));
                            const char* zoom_text = TextFormat("%d%%", zoom_levels[state->grid_zoom]);
                            GuiLabel(LayoutCenter(zoom_text_bounds, GetTextWidth(zoom_text) + 2, zoom_text_bounds.height), zoom_text);

                            if (state->grid_zoom <= zoom_level_min) {
                                GuiDisable();
                            }
                            if ((GuiButton(LayoutOpposite(), "-") || GuiIsKeyPressed(KEY_KP_SUBTRACT)) && state->grid_zoom > zoom_level_min) {
                                state->grid_zoom--;
                            }
                            GuiEnable();
                        }
                        LayoutEnd();

                        int old_text_size = GuiGetStyle(DEFAULT, TEXT_SIZE);
                        GuiSetStyle(DEFAULT, TEXT_SIZE, old_text_size * zoom_levels[state->grid_zoom] / 100);

                        int cell_size = 2 * GuiGetStyle(DEFAULT, TEXT_SIZE);
                        Rectangle grid_bounds = {
                            .x = 0,
                            .y = 0,
                            .width = current_board->history->cols * cell_size,
                            .height = current_board->history->rows * cell_size,
                        };
                        Rectangle grid_view;
                        GuiScrollPanel(LayoutDefault(), NULL, grid_bounds, &state->grid_scroll, &grid_view);

                        BeginScissorMode(grid_view.x, grid_view.y, grid_view.width, grid_view.height);

                        TD_FOREACH(current_board, cursor) {
                            Rectangle cell_bounds = {
                                .x = grid_view.x + state->grid_scroll.x + cursor.col * cell_size,
                                .y = grid_view.y + state->grid_scroll.y + cursor.row * cell_size,
                                .width = cell_size,
                                .height = cell_size,
                            };

                            if (cursor.cell->active) {
                                DrawRectangleRec(cell_bounds, ACTIVE_CELL_COLOR);
                            } else if (cursor.cell->input_kind == CELL_INPUT_A || cursor.cell->input_kind == CELL_INPUT_B) {
                                DrawRectangleRec(cell_bounds, INPUT_CELL_COLOR);
                                if (cursor.cell->input_kind == CELL_INPUT_A) {
                                    DrawText("A", cell_bounds.x + 4, cell_bounds.y + 2, cell_size / 4, BROWN);
                                } else if (cursor.cell->input_kind == CELL_INPUT_B) {
                                    DrawText("B", cell_bounds.x + 4, cell_bounds.y + 2, cell_size / 4, BROWN);
                                }
                            } else if (cursor.cell->kind == CELL_STOP) {
                                DrawRectangleRec(cell_bounds, STOP_CELL_COLOR);
                                DrawText("S", cell_bounds.x + 4, cell_bounds.y + 2, cell_size / 4, RED);
                            }

                            switch (cursor.cell->kind) {
                            case CELL_EMPTY:
                            case CELL_STOP:
                                DrawCircle(cell_bounds.x + cell_size / 2, cell_bounds.y + cell_size / 2, cell_size / 16, LIGHTGRAY);
                                break;
                            case CELL_NUMBER: {
                                const char* text = TextFormat("%d", cursor.cell->value);
                                GuiLabel(LayoutCenter(cell_bounds, GetTextWidth(text) + 2, cell_bounds.height), text);
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
                                GuiLabel(LayoutCenter(cell_bounds, GetTextWidth(text) + 2, cell_bounds.height), text);
                                break;
                            }
                            default: {
                                DrawRectangleRec(cell_bounds, RED);
                                GuiLabel(cell_bounds, td_cell_kind_name(cursor.cell->kind));
                                break;
                            }
                            }
                        }

                        for (size_t x = 0; x <= state->history.cols; ++x) {
                            DrawLine(grid_view.x + state->grid_scroll.x + x * cell_size,
                                     grid_view.y + state->grid_scroll.y,
                                     grid_view.x + state->grid_scroll.x + x * cell_size,
                                     grid_view.y + state->grid_scroll.y + state->history.rows * cell_size,
                                     LIGHTGRAY);
                        }
                        for (size_t y = 0; y <= state->history.rows; ++y) {
                            DrawLine(grid_view.x + state->grid_scroll.x,
                                     grid_view.y + state->grid_scroll.y + y * cell_size,
                                     grid_view.x + state->grid_scroll.x + state->history.cols * cell_size,
                                     grid_view.y + state->grid_scroll.y + y * cell_size,
                                     LIGHTGRAY);
                        }

                        EndScissorMode();

                        GuiSetStyle(DEFAULT, TEXT_SIZE, old_text_size);
                    }
                    LayoutEnd();
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
    UI_State state = {0};
    state.grid_zoom = 3;
    GuiFileDialogInit(&state.open_dialog, "Open 3dl Program", "./examples", ".3dl");

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
    InitWindow(16*90, 9*90, PROGRAM_TITLE);
    SetTargetFPS(60);

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    SetExitKey(0);

    /*const char* program =*/ nob_shift_args(&argc, &argv);
    if (argc >= 1) {
        const char* filename = nob_shift_args(&argc, &argv);

        if (argc > 0) {
            state.gui_input_a = atoi(nob_shift_args(&argc, &argv));
        }

        if (argc > 0) {
            state.gui_input_b = atoi(nob_shift_args(&argc, &argv));
        }

        state.gui_input_a = state.history.input_a;
        state.gui_input_b = state.history.input_b;
        load_file(&state, filename);
    }

    while (!state.close_requested && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        GuiFileDialogCheck(&state.open_dialog);

        if (!state.history.loaded) {
            initial_screen(&state);
        } else {
            run_screen(&state);
        }

        if (GuiFileDialog(&state.open_dialog)) {
            load_file(&state, GuiFileDialogFileName(&state.open_dialog));
        }

        EndDrawing();
    }

    CloseWindow();
    GuiFileDialogFree(&state.open_dialog);

    return 0;
}