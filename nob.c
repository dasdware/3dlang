#define NOB_IMPLEMENTATION
#include "./include/nob.h"

#define BUILD_DIR "build"
#define BUILD_EXTENSION ".exe"
#define BUILD_OUTPUT(output) "." NOB_PATH_DELIM_STR BUILD_DIR NOB_PATH_DELIM_STR output BUILD_EXTENSION

#define RAYLIB_SRC_DIR "3rdparty" NOB_PATH_DELIM_STR "raylib" NOB_PATH_DELIM_STR "src"
#define RAYLIB_BUILD_DIR BUILD_DIR NOB_PATH_DELIM_STR "raylib"

#define RAYGUI_SRC_DIR "3rdparty" NOB_PATH_DELIM_STR "raygui" NOB_PATH_DELIM_STR "src"

#define _3D_TARGET "3d"
#define _3D_OUTPUT BUILD_OUTPUT(_3D_TARGET)

#define RAYLIB_TARGET "raylib"


void gcc(Nob_Cmd* cmd) {
    nob_cmd_append(cmd, "gcc");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-ggdb", "-static");
    nob_cmd_append(cmd, "-I./include/");
    nob_cmd_append(cmd, "-I." NOB_PATH_DELIM_STR RAYGUI_SRC_DIR);
    nob_cmd_append(cmd, "-I." NOB_PATH_DELIM_STR RAYLIB_SRC_DIR);
    nob_cmd_append(cmd, "-L." NOB_PATH_DELIM_STR RAYLIB_BUILD_DIR);
}

bool target_raylib() {
    static const char *raylib_modules[] = {
        "rcore",
        "raudio",
        "rglfw",
        "rmodels",
        "rshapes",
        "rtext",
        "rtextures",
        "utils",
    };

    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_File_Paths object_files = {0};

    if (!nob_mkdir_if_not_exists(RAYLIB_BUILD_DIR)) {
        nob_return_defer(false);
    }

    Nob_Procs procs = {0};
    for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path = nob_temp_sprintf(RAYLIB_SRC_DIR NOB_PATH_DELIM_STR "%s.c", raylib_modules[i]);
        const char *output_path = nob_temp_sprintf(RAYLIB_BUILD_DIR NOB_PATH_DELIM_STR "%s.o", raylib_modules[i]);

        nob_da_append(&object_files, output_path);

        if (nob_needs_rebuild(output_path, &input_path, 1)) {
            cmd.count = 0;
            gcc(&cmd);
            // nob_cmd_append(&cmd, "x86_64-w64-mingw32-gcc");
            // nob_cmd_append(&cmd, "-ggdb", "-DPLATFORM_DESKTOP", "-fPIC", "-DSUPPORT_FILEFORMAT_FLAC=1");
            nob_cmd_append(&cmd, "-DPLATFORM_DESKTOP");
            nob_cmd_append(&cmd, "-fPIC");
            nob_cmd_append(&cmd, "-I." NOB_PATH_DELIM_STR RAYLIB_SRC_DIR NOB_PATH_DELIM_STR "external" NOB_PATH_DELIM_STR "glfw" NOB_PATH_DELIM_STR "include");
            nob_cmd_append(&cmd, "-c", input_path);
            nob_cmd_append(&cmd, "-o", output_path);

            Nob_Proc proc = nob_cmd_run_async(cmd);
            nob_da_append(&procs, proc);
        }

        // break;
    }
    cmd.count = 0;

    if (!nob_procs_wait(procs)) nob_return_defer(false);

    const char *libraylib_path = RAYLIB_BUILD_DIR NOB_PATH_DELIM_STR "libraylib.a";

    if (nob_needs_rebuild(libraylib_path, object_files.items, object_files.count)) {
        // gcc(&cmd);
        nob_cmd_append(&cmd, "ar", "-crs", libraylib_path);
        // nob_cmd_append(&cmd, "x86_64-w64-mingw32-ar", "-crs", libraylib_path);
        for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path = nob_temp_sprintf("%s/%s.o", RAYLIB_BUILD_DIR, raylib_modules[i]);
            nob_cmd_append(&cmd, input_path);
        }
        if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);
    }

defer:
    nob_cmd_free(cmd);
    nob_da_free(object_files);
    return result;
}

bool target_3d(int *argc, char*** argv) {
    if (!target_raylib()) exit(1);

    Nob_Cmd cmd = {0};
    bool result = true;

    cmd.count = 0;
    gcc(&cmd);
    nob_cmd_append(&cmd, "-o", _3D_OUTPUT);
    nob_cmd_append(&cmd, "./src/3d.c");
    nob_cmd_append(&cmd, "./src/3dl.c");
    nob_cmd_append(&cmd, "-lraylib");
    nob_cmd_append(&cmd, "-lgdi32");
    nob_cmd_append(&cmd, "-lwinmm");
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
    } else if (strcmp(target, RAYLIB_TARGET) == 0) {
        if (!target_raylib()) exit(1);
    } else {
        nob_log(NOB_ERROR, "Invalid target `%s`.", target);
        return 1;
    }

    return 0;
}