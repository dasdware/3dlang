#define NOB_IMPLEMENTATION
#include "./include/nob.h"

#define BUILD_DIR "build"
#define BUILD_EXTENSION ".exe"
#define BUILD_OUTPUT(output) "." NOB_PATH_DELIM_STR BUILD_DIR NOB_PATH_DELIM_STR output BUILD_EXTENSION

#define _3D_TARGET "3d"
#define _3D_OUTPUT BUILD_OUTPUT(_3D_TARGET)

void gcc(Nob_Cmd* cmd) {
    nob_cmd_append(cmd, "gcc");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-ggdb", "-static");
    nob_cmd_append(cmd, "-I./include/");
    nob_cmd_append(cmd, "-I./include/curl");
    nob_cmd_append(cmd, "-I./include/raylib");
    nob_cmd_append(cmd, "-L./libs/win32");
}

bool target_3d(int *argc, char*** argv) {
    Nob_Cmd cmd = {0};
    bool result = true;

    cmd.count = 0;
    gcc(&cmd);
    nob_cmd_append(&cmd, "-o", _3D_OUTPUT);
    nob_cmd_append(&cmd, "./src/3d.c");
    nob_cmd_append(&cmd, "-lraylibdll");
    if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

    cmd.count = 0;
    nob_cmd_append(&cmd, _3D_OUTPUT);
    while (*argc > 0) {
        nob_cmd_append(&cmd, nob_shift_args(argc, argv));
    }
    if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

defer:
    nob_cmd_free(cmd);
    return result;
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    nob_mkdir_if_not_exists(BUILD_DIR);

    Nob_String_View program = nob_sv_filename_of(nob_sv_shift_args(&argc, &argv));
    printf(SV_Fmt" - Buildtool for 3DLang\n", SV_Arg(program));

    if (argc == 0) {
        nob_log(NOB_ERROR, "No target given.");
        return 1;
    }

    const char* target = nob_shift_args(&argc, &argv);
    if (strcmp(target, _3D_TARGET) == 0) {
        if (!target_3d(&argc, &argv)) exit(1);
    } else {
        nob_log(NOB_ERROR, "Invalid target `%s`.", target);
        return 1;
    }

    return 0;
}