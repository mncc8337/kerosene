#include "kshell.h"
#include "system.h"
#include "syscall.h"
#include "mem.h"
#include "kbd.h"
#include "video.h"
#include "filesystem.h"
#include "pit.h"
#include "process.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "time.h"

#define MAX_INPUT 1024

typedef enum {
    ERR_SHELL_SUCCESS,
    ERR_SHELL_NO_FS,
    ERR_SHELL_NOT_FOUND,
    ERR_SHELL_TARGET_NOT_FOUND,
    ERR_SHELL_NOT_A_DIR
} SHELL_ERR;

static bool shell_running = true;

static char* input;
static unsigned input_len = 0;

static char* last_input;

static char* current_font = NULL;

static int fsid = -1;

// a stack that contain the path of current dir
static fs_node_t* node_stack;
static unsigned node_stack_offset = 0;

static fs_node_t* node_stack_top() {
    return node_stack + node_stack_offset;
}
static bool node_stack_push(fs_node_t node) {
    if(node_stack_offset == NODE_STACK_MAX_LENGTH - 1)
        return false;

    node_stack[++node_stack_offset] = node;
    return true;
}
static void node_stack_pop() {
    if(node_stack_offset > 0) node_stack_offset--;
}

static FS_ERR list_dir(fs_node_t* parent, int max_depth, bool show_hidden, int* indent_level) {
    if((*indent_level) == max_depth) return ERR_FS_SUCCESS;
    directory_iterator_t diriter;
    fs_setup_directory_iterator(&diriter, parent);

    fs_node_t node;

    FS_ERR last_err;
    while(!(last_err = fs_read_dir(&diriter, &node))) {
        if((FS_NODE_IS_HIDDEN(node) || node.name[0] == '.') && !show_hidden) continue;
        (*indent_level)++;

        for(int i = 0; i < (*indent_level)-1; i++)
            printf("|   ");
        printf("|---");

        if(FS_NODE_IS_DIR(node)) video_set_attr(video_rgb(VIDEO_LIGHT_BLUE), video_rgb(VIDEO_BLACK));
        puts(node.name);
        if(FS_NODE_IS_DIR(node)) video_set_attr(video_rgb(VIDEO_LIGHT_GREY), video_rgb(VIDEO_BLACK));

        if(FS_NODE_IS_DIR(node) && !strcmp(node.name, ".") && !strcmp(node.name, "..")) {
            FS_ERR err = list_dir(&node, max_depth, show_hidden, indent_level);
            if(err) return err;
        }

        (*indent_level)--;
    }

    if(last_err != ERR_FS_EOF)
        return last_err;
    return ERR_FS_SUCCESS;
}
static int path_find_last_node(char* path, fs_node_t* parent, fs_node_t* node) {
    *parent = *(node_stack_top());

    unsigned org_fs = fsid;

    if(path[0] == '(') {
        // disk specified
        char diskid_string[6];
        for(unsigned i = 0; i < 5; i++) {
            if(path[i + 1] != ')') {
                diskid_string[i] = path[i + 1];
            }
            else {
                diskid_string[i] = '\0';
                break;
            }
        }

        unsigned diskid = atoi(diskid_string);
        if(vfs_is_fs_available(diskid)) {
            shell_set_fs(diskid);
        }
        else return ERR_SHELL_NO_FS;
        *node = node_stack[0];
        *parent = node_stack[0];
        path += strlen(diskid_string) + 2;
    }
    if(path[0] == '/') {
        // root specified
        *node = node_stack[0];
        *parent = node_stack[0];
        path++;
    }

    char* nodename = strtok(path, "/");
    while(nodename != NULL) {
        // root dir does not have the . and .. dir so we need to handle it differently
        if((strcmp(nodename, ".") || strcmp(nodename, ".."))
                && parent->name[0] == '/')
            *node = node_stack[0];
        else {
            FS_ERR find_err = fs_find(parent, nodename, node);
            if(find_err) {
                memcpy(node->name, nodename, strlen(nodename)+1);
                if(strtok(NULL, "/") != NULL) {
                    shell_set_fs(org_fs);
                    return ERR_SHELL_NOT_FOUND;
                }
                shell_set_fs(org_fs);
                return ERR_SHELL_TARGET_NOT_FOUND;
            }
        }
        nodename = strtok(NULL, "/");
        if(nodename != NULL && !FS_NODE_IS_DIR(*node)) { // this is illegal
            shell_set_fs(org_fs);
            return ERR_SHELL_NOT_A_DIR;
        }

        if(nodename != NULL) *parent = *node;
    }

    shell_set_fs(org_fs);
    return ERR_SHELL_SUCCESS;
}

static void help(char* arg) {
    if(arg == NULL) {
        puts("help clear . echo clocks ls read cd mkdir rm touch write append mv cp stat pwd datetime beep draw panic catproc sleep exit");
    }
    else {
        arg = strtok(arg, " ");
        if(strcmp(arg, "help")) puts("print help\nhelp <command name, optional>");
        else if(strcmp(arg, "clear")) puts("clear screen\nclear <no-args>");
        else if(strcmp(arg, ".")) puts("execute file content as shell command\n. <path>");
        else if(strcmp(arg, "echo")) puts("print string\necho <string>");
        else if(strcmp(arg, "clocks")) puts("print current clocks\nclocks <no-args>");
        else if(strcmp(arg, "ls")) {
            puts(
                "list directory contents\n"
                "ls <args> <directory>\n"
                "available arg:\n"
                "    -a          show hidden\n"
                "    -d <num>    tree depth"
            );
        }
        else if(strcmp(arg, "read")) puts("print file content\nread <path>");
        else if(strcmp(arg, "cd")) puts("change directory\ncd <path>");
        else if(strcmp(arg, "mkdir")) puts("make a directory\nmkdir <path>");
        else if(strcmp(arg, "rm")) puts("remove a file or directory\nrm <path>");
        else if(strcmp(arg, "touch")) puts("make an empty file\ntouch <path>");
        else if(strcmp(arg, "write")) puts("write to a file\nwrite <path>");
        else if(strcmp(arg, "append")) puts("append to a file\nappend <path>");
        else if(strcmp(arg, "mv")) puts("move a file or directory\nmv <source-path> <destination-path>");
        else if(strcmp(arg, "cp")) puts("copy a file or directory\ncp <source-path> <destination-path>");
        else if(strcmp(arg, "stat")) puts("print a file or directory stat\nstat <path>");
        else if(strcmp(arg, "pwd")) puts("print current directory absolute path\npwd <no-args>");
        else if(strcmp(arg, "datetime")) puts("print current datetime\ndatetime <no-args>");
        else if(strcmp(arg, "beep")) puts("make a beep\nbeep <frequency> <duration, optional>");
        else if(strcmp(arg, "draw")) {
            puts(
                "draw stuffs\n"
                "draw <mode> <args>\n"
                "available mode:\n"
                "    line  <arg> = x0 y0 x1 y1 color\n"
                "    rect  <arg> = x0 y0 x1 y1 color\n"
                "    circ  <arg> = x y r color"
            );
        }
        else if(strcmp(arg, "panic")) puts("causes the kernel to panic\npanic <no-args>");
        else if(strcmp(arg, "catproc")) puts("print all processes and their info\ncatproc <no-args>");
        else if(strcmp(arg, "sleep")) puts("halt for an ammount of time\nsleep <ticks>");
        else if(strcmp(arg, "loadfont")) puts("load new font\nloadfont <psf-file>");
        else if(strcmp(arg, "exit")) puts("quit shell\nexit <no-arg>");
    }
}

static void clear(char* arg) {
    (void)(arg);
    video_cls(video_rgb(VIDEO_BLACK));
}

static void run_sh(char* args) {
    if(args == NULL) {
        puts("no file input");
        return;
    }

    args = strtok(args, " ");

    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(args, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND || FS_NODE_IS_DIR(node)) {
        printf("no such file '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    file_descriptor_entry_t f;
    file_open(&f, &node, "r");
    char chr;
    input_len = 0;
    input[0] = '\0';
    FS_ERR last_err;
    while((last_err = file_read(&f, (uint8_t*)(&chr), 1)) == ERR_FS_SUCCESS) {
        if(chr != '\n' && chr != ';')
            input[input_len++] = chr;
        else {
            input[input_len] = '\0';
            shell_process_prompt(input, input_len);
            input_len = 0;
            input[0] = '\0';
        }
    }
    if(last_err != ERR_FS_EOF) {
        file_close(&f);
        printf("failed to read. error code %d\n", last_err);
        return;
    }

    if(input_len > 0 && input[0] != '\0') {
        input[input_len] = '\0';
        shell_process_prompt(input, input_len);
    }

    file_close(&f);
}

static void echo(char* args) {
    puts(args);
}

static void clocks(char* args) {
    (void)(args);
    printf("%d\n", clock());
}

static void ls(char* args) {
    fs_node_t* current_node = node_stack_top();
    int max_depth = 1;
    bool show_hidden = false;
    char* ls_name = NULL;

    char* arg = strtok(args, " ");
    while(arg != NULL) {
        if(arg[0] == '-') {
            if(strcmp(arg, "-d")) {
                arg = strtok(NULL, " ");
                if(arg == NULL) {
                    puts("not enough arguement");
                    return;
                }
                max_depth = atoi(arg);
            }
            else if(strcmp(arg, "-a"))
                show_hidden = true;
            else {
                printf("unknown argument: %s\n", arg);
                return;
            }
        }
        else
            ls_name = arg;

        arg = strtok(NULL, " ");
    }

    int indent_level = 0;

    if(ls_name == NULL) {
        list_dir(current_node, max_depth, show_hidden, &indent_level);
        return;
    }
    else {
        fs_node_t node_parent;
        fs_node_t node;
        SHELL_ERR err = path_find_last_node(ls_name, &node_parent, &node);
        if(err == ERR_SHELL_NOT_FOUND
                || err == ERR_SHELL_NOT_A_DIR
                || err == ERR_SHELL_TARGET_NOT_FOUND
                || !FS_NODE_IS_DIR(node)) {
            printf("no such directory '%s'\n", node.name);
            return;
        }
        if(err == ERR_SHELL_NO_FS) {
            puts("invalid fs");
            return;
        }

        list_dir(&node, max_depth, show_hidden, &indent_level);
    }
}

static void read(char* path) {
    if(path == NULL) {
        puts("no file input");
        return;
    }

    path = strtok(path, " ");

    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(path, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND || FS_NODE_IS_DIR(node)) {
        printf("no such file '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    file_descriptor_entry_t f;
    file_open(&f, &node, "r");
    char chr;
    FS_ERR last_err;
    while((last_err = file_read(&f, (uint8_t*)&chr, 1)) == ERR_FS_SUCCESS) {
        putchar(chr);
    }
    file_close(&f);

    if(last_err != ERR_FS_EOF) {
        printf("failed to read, error code %d\n", last_err);
    }
}

static void cd(char* path) {
    if(path == NULL) {
        puts("no path provided");
        return;
    }

    path = strtok(path, " ");

    if(path[0] == '(') {
        // disk specified
        char diskid_string[6];
        for(unsigned i = 0; i < 5; i++) {
            if(path[i + 1] != ')') {
                diskid_string[i] = path[i + 1];
            }
            else {
                diskid_string[i] = '\0';
                break;
            }
        }

        unsigned diskid = atoi(diskid_string);
        if(vfs_is_fs_available(diskid)) {
            shell_set_fs(diskid);
        }
        else {
            printf("no such fs %s\n", diskid_string);
            return;
        }
        path += strlen(diskid_string) + 2;
    }
    if(path[0] == '/') {
        // to rootdir
        node_stack_offset = 0;
        path++;
    }

    char* nodename = strtok(path, "/");
    while(nodename != NULL) {
        if(strcmp(nodename, "..")) node_stack_pop();
        else if(!strcmp(nodename, ".")) {
            fs_node_t tmp;
            FS_ERR find_err = fs_find(node_stack_top(), nodename, &tmp);
            if(find_err) {
                printf("cannot find '%s', error %d\n", nodename, find_err);
                return;
            }
            if(!FS_NODE_IS_DIR(tmp)) {
                printf("no such directory '%s'\n", nodename);
                return;
            }

            bool pushed = node_stack_push(tmp);
            if(!pushed) {
                printf("reached node stack limit, cannot cd into '%s'\n", nodename);
                return;
            }
        }

        nodename = strtok(NULL, "/");
    }
}

static void mkdir(char* path) {
    fs_node_t* current_node = node_stack_top();

    if(path == NULL) {
        puts("no name provided");
        return;
    }

    path = strtok(path, " ");

    fs_node_t _curr_node = *current_node;

    if(path[0] == '/') {
        _curr_node = node_stack[0];
        path++;
    }

    char* dirname = strtok(path, "/");
    while(dirname != NULL) {
        fs_node_t node;
        FS_ERR find_err = fs_find(&_curr_node, dirname, &node);
        if(!find_err) {
            printf("a file or directory with name '%s' has already existed\n", dirname);
            return;
        }
        if(find_err != ERR_FS_NOT_FOUND) {
            printf("failed to search for '%s'. error %d\n", dirname, find_err);
            return;
        }

        fs_node_t newdir;
        FS_ERR mkdir_err = fs_mkdir(&_curr_node, dirname, &newdir);
        if(mkdir_err) {
            printf("failed to create directory '%s', error %d\n", dirname, mkdir_err);
            return;
        }
        _curr_node = newdir;
        dirname = strtok(NULL, "/");
    }
}

static void rm(char* path) {
    if(path == NULL) {
        puts("no name provided");
        return;
    }

    path = strtok(path, " ");
    
    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR serr = path_find_last_node(path, &node_parent, &node);
    if(serr == ERR_SHELL_NOT_FOUND || serr == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(serr == ERR_SHELL_TARGET_NOT_FOUND) {
        printf("no such file or directory '%s'\n", node.name);
        return;
    }
    if(serr == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    FS_ERR err = fs_remove(&node_parent, &node);
    if(err != ERR_FS_SUCCESS)
        printf("cannot remove '%s'. error code %d\n", node.name, err);
}

static void touch(char* path) {
    if(path == NULL) {
        puts("no name provided");
        return;
    }

    path = strtok(path, " ");

    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(path, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("'%s' is not a directory\n", node.name);
        return;
    }
    if(err == ERR_SHELL_SUCCESS) {
        // we dont want it to find a valid node
        printf("a file or directory with name '%s' has already existed\n", path);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }
    // at this point the error should be ERR_SHELL_TARGET_NOT_FOUND
    // which is what we wanted

    FS_ERR touch_err =  fs_touch(&node_parent, node.name, &node);
    if(touch_err) printf("cannot create '%s', error %d\n", path, touch_err);
}

static void write(char* path) {
    if(path == NULL) {
        puts("no name provided");
        return;
    }

    path = strtok(path, " ");

    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(path, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND) {
        // try to create one
        FS_ERR touch_err = fs_touch(&node_parent, node.name, &node);
        if(touch_err) {
            printf("cannot create file '%s', error %d\n", path, touch_err);
            return;
        }
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    file_descriptor_entry_t f;
    file_open(&f, &node, "w");

    input_len = 0;
    puts("writing mode. press ESC to exit");

    key_t current_key; current_key.keycode = 1;
    while(current_key.keycode != KBD_KEYCODE_ESC) {
        kbd_wait_key(&current_key);
        if(current_key.released) continue;

        if(current_key.mapped == '\0') continue;


        if(current_key.mapped == '\b' && input_len > 0) {
            putchar(current_key.mapped);
            input_len--;
            continue;
        }

        putchar(current_key.mapped);
        input[input_len++] = current_key.mapped;
        if(current_key.mapped == '\n' || input_len >= MAX_INPUT - 10) {
            file_write(&f, (uint8_t*)input, input_len);
            input_len = 0;
        }
    }

    if(input_len > 0)
        file_write(&f, (uint8_t*)input, input_len);
    file_close(&f);
}

static void append(char* path) {
    if(path == NULL) {
        puts("no name provided");
        return;
    }

    path = strtok(path, " ");

    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(path, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND) {
        printf("no such file file '%s'\n", path);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    file_descriptor_entry_t f;
    file_open(&f, &node, "a");

    input_len = 0;
    puts("writing mode. press ESC to exit");

    key_t current_key; current_key.keycode = 1;
    while(current_key.keycode != KBD_KEYCODE_ESC) {
        kbd_wait_key(&current_key);
        if(current_key.released) continue;

        if(current_key.mapped == '\0') continue;


        if(current_key.mapped == '\b') {
            if(input_len == 0) continue;
            putchar(current_key.mapped);
            input_len--;
            continue;
        }

        putchar(current_key.mapped);
        input[input_len++] = current_key.mapped;
        if(current_key.mapped == '\n' || input_len >= MAX_INPUT - 10) {
            file_write(&f, (uint8_t*)input, input_len);
            input_len = 0;
        }
    }

    if(input_len > 0)
        file_write(&f, (uint8_t*)input, input_len);
    file_close(&f);
}

static void mv(char* args) {
    char* source_path = strtok(args, " ");
    if(source_path == NULL) {
        puts("no source provided");
        return;
    }
    char* target_path = strtok(NULL, " ");
    if(target_path == NULL) {
        puts("no target provided");
        return;
    }

    fs_node_t source_node_parent;
    fs_node_t source_node;
    SHELL_ERR err = path_find_last_node(source_path, &source_node_parent, &source_node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", source_node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND) {
        printf("no such file or directory '%s'\n", source_node.name);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    fs_node_t target_node_parent;
    fs_node_t target_node;
    err = path_find_last_node(target_path, &target_node_parent, &target_node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", target_node.name);
        return;
    }
    if(err == ERR_SHELL_SUCCESS) {
        if(!FS_NODE_IS_DIR(target_node)) {
            printf("a file with name '%s' has already existed\n", target_node.name);
            return;
        }
        else {
            target_node_parent = target_node;
            memcpy(target_node.name, source_node.name, strlen(source_node.name)+1);
        }
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    FS_ERR ferr = fs_move(&source_node, &target_node_parent, target_node.name);
    if(ferr != ERR_FS_SUCCESS)
        printf("failed to move %s to %s with name %s. error code %d\n", source_node.name, target_node_parent.name, target_node.name, ferr);
}

static void cp(char* args) {
    char* source_path = strtok(args, " ");
    if(source_path == NULL) {
        puts("no source provided");
        return;
    }
    char* target_path = strtok(NULL, " ");
    if(target_path == NULL) {
        puts("no target provided");
        return;
    }

    fs_node_t source_node_parent;
    fs_node_t source_node;
    SHELL_ERR err = path_find_last_node(source_path, &source_node_parent, &source_node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", source_node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND) {
        printf("no such file or directory '%s'\n", source_node.name);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    fs_node_t target_node_parent;
    fs_node_t target_node;
    err = path_find_last_node(target_path, &target_node_parent, &target_node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", target_node.name);
        return;
    }
    if(err == ERR_SHELL_SUCCESS) {
        if(!FS_NODE_IS_DIR(target_node)) {
            printf("a file with name '%s' has already existed\n", target_node.name);
            return;
        }
        else {
            target_node_parent = target_node;
            memcpy(target_node.name, source_node.name, strlen(source_node.name)+1);
        }
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    fs_node_t copied;
    FS_ERR ferr = fs_copy(&source_node, &target_node_parent, &copied, target_node.name);
    if(ferr != ERR_FS_SUCCESS)
        printf("failed to copy %s to %s with name %s. error code %d\n", source_node.name, target_node_parent.name, target_node.name, ferr);
}

static void stat(char* path) {
    if(path == NULL) {
        puts("no input");
        return;
    }

    path = strtok(path, " ");

    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(path, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND) {
        printf("no such file '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    printf("filesystem: %s\n", (node.fs->type == 1 ? "FAT32" : (node.fs->type == 2 ? "ext2" : ( node.fs->type == 3 ? "ramfs" :"unknown"))));
    printf("parent: '%s'\n", node.parent_node->name);
    printf("type: %s\n", (FS_NODE_IS_DIR(node) ? "directory" : "file"));
    printf("hidden: %s\n", (FS_NODE_IS_HIDDEN(node) ? "true" : "false"));
    printf("size: %d bytes\n", node.size);

    printf("ram node addr: %x\n", node.ramfs.node_addr);

    printf("fat32 start cluster: %x\n", node.fat32.start_cluster);
    printf("fat32 parent cluster: %x\n", node.fat32.parent_cluster);

    struct tm t_dump = gmtime(&(node.creation_timestamp));
    printf("creation timestamp: %d/%d/%d %d:%d:%d.%d\n",
           t_dump.tm_mday, t_dump.tm_mon, t_dump.tm_year,
           t_dump.tm_hour, t_dump.tm_min, t_dump.tm_sec, node.creation_milisecond);
    t_dump = gmtime(&(node.accessed_timestamp));
    printf("last accessed timestamp: %d/%d/%d %d:%d:%d\n",
           t_dump.tm_mday, t_dump.tm_mon, t_dump.tm_year,
           t_dump.tm_hour, t_dump.tm_min, t_dump.tm_sec);
    t_dump = gmtime(&(node.modified_timestamp));
    printf("last modified timestamp: %d/%d/%d %d:%d:%d\n",
           t_dump.tm_mday, t_dump.tm_mon, t_dump.tm_year,
           t_dump.tm_hour, t_dump.tm_min, t_dump.tm_sec);
}

static void pwd(char* args) {
    (void)(args);

    if(node_stack_offset == 0) {
        puts("/");
        return;
    }

    for(unsigned int i = 1; i <= node_stack_offset; i++)
        printf("/%s", node_stack[i].name);
    putchar('\n');
}

static void datetime(char* arg) {
    (void)(arg);
    time_t curr_time = time(NULL);
    struct tm t = gmtime(&curr_time);

    printf("%d:%d:%d, ", t.tm_hour, t.tm_min, t.tm_sec);
    switch(t.tm_wday) {
        case 1:
            printf("mon ");
            break;
        case 2:
            printf("tue ");
            break;
        case 3:
            printf("wed ");
            break;
        case 4:
            printf("thu ");
            break;
        case 5:
            printf("fri ");
            break;
        case 6:
            printf("sat ");
            break;
        case 0:
            printf("sun ");
            break;
    }
    printf("%d/%d/%d\n", t.tm_mday, t.tm_mon, t.tm_year);
    printf("days since 1st january: %d\n", t.tm_yday);
    printf("seconds since epoch: %d\n", curr_time);
}

static void beep(char* arg) {
    char* frq_str = strtok(arg, " ");
    if(frq_str == NULL) {
        puts("no frequency provided");
        return;
    }
    int frq = atoi(frq_str);

    char* dur_str = strtok(NULL, " ");
    int dur;
    if(dur_str == NULL) {
        dur = 1000;
    }
    else dur = atoi(dur_str);

    pit_beep_start();
    pit_beep(frq);
    SYSCALL_1P(SYSCALL_SLEEP, frq_str, dur * TIMER_FREQUENCY / 1000);
    pit_beep_stop();
}

static void _color(char* rgb, int* r, int* g, int* b) {
    char* r_str;
    char* g_str;
    char* b_str;

    r_str = strtok(rgb, ",");
    if(r_str == NULL) goto return_error;
    *r = atoi(r_str);

    g_str = strtok(NULL, ",");
    if(g_str == NULL) goto return_error;
    *g = atoi(g_str);

    b_str = strtok(NULL, ",");
    if(b_str == NULL) goto return_error;
    *b = atoi(b_str);
    return;

    return_error:
    *r = -1;
    *g = -1;
    *b = -1;
}
static void draw(char* arg) {
    if(!video_using_framebuffer()) {
        puts("framebuffer not available");
        return;
    }

    char* mode = strtok(arg, " ");
    if(mode == NULL) {
        puts("no mode provided");
        return;
    }

    if(strcmp(mode, "line")) {
        char* x0_str = strtok(NULL, " ");
        char* y0_str = strtok(NULL, " ");
        char* x1_str = strtok(NULL, " ");
        char* y1_str = strtok(NULL, " ");
        char* color_str = strtok(NULL, " ");

        int x0, y0, x1, y1, color;

        if(x0_str == NULL) {
            puts("no x0 provided");
            return;
        }
        if(y0_str == NULL) {
            puts("no y0 provided");
            return;
        }
        if(x1_str == NULL) {
            puts("no x1 provided");
            return;
        }
        if(y1_str == NULL) {
            puts("no y1 provided");
            return;
        }

        if(color_str == NULL) color = video_framebuffer_rgb(255, 255, 255);
        else {
            int r, g, b;
            _color(color_str, &r, &g, &b);
            if(b == -1) {
                puts("color not provided");
                return;
            }

            color = video_framebuffer_rgb(r, g, b);
        }

        x0 = atoi(x0_str);
        y0 = atoi(y0_str);
        x1 = atoi(x1_str);
        y1 = atoi(y1_str);

        video_framebuffer_draw_line(x0, y0, x1, y1, color);
    }
    else if(strcmp(mode, "rect")) {
        char* x0_str = strtok(NULL, " ");
        char* y0_str = strtok(NULL, " ");
        char* x1_str = strtok(NULL, " ");
        char* y1_str = strtok(NULL, " ");
        char* color_str = strtok(NULL, " ");

        int x0, y0, x1, y1, color;

        if(x0_str == NULL) {
            puts("no x0 provided");
            return;
        }
        if(y0_str == NULL) {
            puts("no y0 provided");
            return;
        }
        if(x1_str == NULL) {
            puts("no x1 provided");
            return;
        }
        if(y1_str == NULL) {
            puts("no y1 provided");
            return;
        }

        if(color_str == NULL) color = video_framebuffer_rgb(255, 255, 255);
        else {
            int r, g, b;
            _color(color_str, &r, &g, &b);
            if(b == -1) {
                puts("color not provided");
                return;
            }

            color = video_framebuffer_rgb(r, g, b);
        }

        x0 = atoi(x0_str);
        y0 = atoi(y0_str);
        x1 = atoi(x1_str);
        y1 = atoi(y1_str);

        video_framebuffer_fill_rectangle(x0, y0, x1, y1, color);
    }
    else if(strcmp(mode, "circ")) {
        char* x_str = strtok(NULL, " ");
        char* y_str = strtok(NULL, " ");
        char* r_str = strtok(NULL, " ");
        char* color_str = strtok(NULL, " ");

        int x, y, r, color;

        if(x_str == NULL) {
            puts("no x provided");
            return;
        }
        if(y_str == NULL) {
            puts("no y provided");
            return;
        }
        if(r_str == NULL) {
            puts("no r provided");
            return;
        }

        if(color_str == NULL) color = video_framebuffer_rgb(255, 255, 255);
        else {
            int r, g, b;
            _color(color_str, &r, &g, &b);
            if(b == -1) {
                puts("color not provided");
                return;
            }

            color = video_framebuffer_rgb(r, g, b);
        }

        x = atoi(x_str);
        y = atoi(y_str);
        r = atoi(r_str);

        video_framebuffer_draw_circle(x, y, r, color);
    }
    else {
        puts("mode not recognised");
    }
}

static void panic(char* arg) {
    (void)(arg);
   kernel_panic(NULL);
}

static void print_proc(process_t* proc) {
    if(!proc) return;
    printf(
        "process %d:\n"
        "    priority: %d\n"
        "    state: %s\n"
        "    alive ticks: %d\n"
        ,
        proc->id, proc->priority,
        proc->state == PROCESS_STATE_ACTIVE ? "active" : 
        proc->state == PROCESS_STATE_READY ? "ready" : "sleep",
        proc->alive_ticks
    );

    if(proc->state == PROCESS_STATE_SLEEP)
        printf("    sleep ticks: %d\n", proc->sleep_ticks);
}
static void catproc(char* arg) {
    (void)(arg);

    // print all processes

    print_proc(scheduler_get_current_process());

    process_t* proc = scheduler_get_ready_processes();
    while(proc) {
        print_proc(proc);
        proc = proc->next;
    }

    proc = scheduler_get_sleep_processes();
    while(proc) {
        print_proc(proc);
        proc = proc->next;
    }
}

static void sleep(char* arg) {
    char* ticks_str = strtok(arg, " ");
    if(!ticks_str) {
        puts("no duration provided");
        return;
    }

    unsigned ticks = atoi(ticks_str);
    SYSCALL_1P(SYSCALL_SLEEP, ticks_str, ticks);
}

static void loadfont(char* path) {
    if(!video_using_framebuffer()) {
        puts("not supported in current video mode");
        return;
    }

    if(path == NULL) {
        puts("no file input");
        return;
    }

    path = strtok(path, " ");

    fs_node_t node_parent;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(path, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND || FS_NODE_IS_DIR(node)) {
        printf("no such file '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_NO_FS) {
        puts("invalid fs");
        return;
    }

    char* new_font = kmalloc(node.size);
    if(!new_font) {
        puts("not enough memory to load the font");
        return;
    }

    file_descriptor_entry_t f;
    file_open(&f, &node, "r");
    FS_ERR read_err = file_read(&f, (uint8_t*)new_font, node.size);
    file_close(&f);

    if(read_err != ERR_FS_EOF && read_err != ERR_FS_SUCCESS) {
        kfree(new_font);
        printf("read file failed, error code %d\n", read_err);
        return;
    }

    bool font_err = video_framebuffer_set_font(new_font);
    if(font_err) {
        printf("failed to load font %s\n", node.name);
        kfree(new_font);
        return;
    }

    printf("loaded new font %s\n", node.name);

    if(current_font) kfree(current_font);
    current_font = new_font;
}

static void exit(char* arg) {
    (void)(arg);

    shell_running = false;
}

void shell_process_prompt(char* prompts, unsigned prompts_len) {
    char* prompt = strtok(prompts, ";\n");
    unsigned tot_len = 0;
    while(prompt != NULL && tot_len < prompts_len) {
        unsigned prompt_len = strlen(prompt);
        char* cmd_name = strtok(prompt, " ");
        char* remain_arg = cmd_name + strlen(cmd_name) + 1;
        while(remain_arg[0] == '\0' || remain_arg[0] == ' ') remain_arg++;
        if(remain_arg - cmd_name >= (signed)prompt_len) remain_arg = NULL;

        if(strcmp(cmd_name, "help")) help(remain_arg);
        else if(strcmp(cmd_name, "clear")) clear(remain_arg);
        else if(strcmp(cmd_name, ".")) run_sh(remain_arg);
        else if(strcmp(cmd_name, "echo")) echo(remain_arg);
        else if(strcmp(cmd_name, "clocks")) clocks(remain_arg);
        else if(strcmp(cmd_name, "ls")) ls(remain_arg);
        else if(strcmp(cmd_name, "read")) read(remain_arg);
        else if(strcmp(cmd_name, "cd")) cd(remain_arg);
        else if(strcmp(cmd_name, "mkdir")) mkdir(remain_arg);
        else if(strcmp(cmd_name, "rm")) rm(remain_arg);
        else if(strcmp(cmd_name, "touch")) touch(remain_arg);
        else if(strcmp(cmd_name, "write")) write(remain_arg);
        else if(strcmp(cmd_name, "append")) append(remain_arg);
        else if(strcmp(cmd_name, "mv")) mv(remain_arg);
        else if(strcmp(cmd_name, "cp")) cp(remain_arg);
        else if(strcmp(cmd_name, "stat")) stat(remain_arg);
        else if(strcmp(cmd_name, "pwd")) pwd(remain_arg);
        else if(strcmp(cmd_name, "datetime")) datetime(remain_arg);
        else if(strcmp(cmd_name, "beep")) beep(remain_arg);
        else if(strcmp(cmd_name, "draw")) draw(remain_arg);
        else if(strcmp(cmd_name, "panic")) panic(remain_arg);
        else if(strcmp(cmd_name, "catproc")) catproc(remain_arg);
        else if(strcmp(cmd_name, "loadfont")) loadfont(remain_arg);
        else if(strcmp(cmd_name, "sleep")) sleep(remain_arg);
        else if(strcmp(cmd_name, "exit")) exit(remain_arg);
        else if(prompt_len == 0); // just skip
        else printf("unknow command '%s'\n", prompt);

        tot_len += prompt_len;
        prompt = strtok(prompt+prompt_len+1, ";\n");
    }
}

static void print_prompt() {
    if(!shell_running) return;

    int w, h;
    if(video_using_framebuffer()) video_framebuffer_get_rowcol(&w, &h);
    else video_get_size(&w, &h);
    if(video_get_cursor() % w != 0)
        putchar('\n');

    putchar('[');
    video_set_attr(video_rgb(VIDEO_GREEN), video_rgb(VIDEO_BLACK));
    printf("kernel@kshell");
    video_set_attr(video_rgb(VIDEO_LIGHT_BLUE), video_rgb(VIDEO_BLACK));
    putchar(' ');
    if(node_stack[node_stack_offset].name[0] == '/') printf("(%d)", fsid); // add fsid when at root
    printf("%s ", node_stack[node_stack_offset].name);
    video_set_attr(video_rgb(VIDEO_LIGHT_GREY), video_rgb(VIDEO_BLACK));
    printf("]$ ");
}

bool shell_init() {
    input = (char*)kmalloc(sizeof(char) * MAX_INPUT);
    last_input = (char*)kmalloc(sizeof(char) * MAX_INPUT);
    node_stack = (fs_node_t*)kmalloc(sizeof(fs_node_t) * NODE_STACK_MAX_LENGTH);

    if(!input || !last_input || !node_stack) {
        if(input) kfree(input);
        if(last_input) kfree(last_input);
        if(node_stack) kfree(node_stack);

        return true;
    }

    return false;
}

void shell_set_fs(int id) {
    node_stack[0] = vfs_getfs(id)->root_node;
    node_stack_offset = 0;
    fsid = id;
}
void shell_start() {
    // default disk
    if(fsid < 0) shell_set_fs(RAMFS_DISK);

    shell_running = true;
    puts("welcome to keroshell");
    puts("type `help` t show all command. `help <command>` to see all available argument.");
    puts("type `exit` to quit shell");
    print_prompt();

    key_t current_key;
    while(shell_running) {
        kbd_wait_key(&current_key);
        if(current_key.released) continue;

        if(current_key.keycode == KBD_KEYCODE_UP) {
            int currpos = video_get_cursor();
            for(int i = 0; i <= (signed)input_len; i++) {
                video_set_cursor(currpos - i);
                video_print_char(' ', -1, -1, -1, false);
            }

            input_len = strlen(last_input);
            memcpy(input, last_input, input_len+1);
            printf(last_input);
        }

        // ignore non printable characters
        if(current_key.mapped == '\0') continue;

        if(current_key.mapped == '\b') {
            if(input_len == 0) continue;
            putchar(current_key.mapped);
            input_len--;
            continue;
        }

        putchar(current_key.mapped);
        if(current_key.mapped != '\n') {
            input[input_len++] = current_key.mapped;
        }
        else {
            input[input_len] = '\0';
            memcpy(last_input, input, input_len+1);

            shell_process_prompt(input, input_len);
            input[0] = '\0';
            input_len = 0;
            print_prompt();
        }
    }

    kfree(input);
    kfree(last_input);
    kfree(node_stack);

    int ret;
    SYSCALL_0P(SYSCALL_KILL_PROCESS, ret);
}
