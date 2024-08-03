#include "kshell.h"
#include "kbd.h"
#include "timer.h"
#include "tty.h"
#include "filesystem.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "timer.h"

static char input[512];
static unsigned int input_len = 0;
static char* current_token;

static fs_node_t current_node;

static volatile bool key_handled = true;
static volatile key_t current_key;
static void kbd_listener(key_t k) {
    if(k.released) return;
    key_handled = false;
    current_key = k;
}

static void tick_listener(unsigned int ticks) {
    (void)(ticks);

    // put some sheduled tasks here
}

static int indent_level = 0;
static int max_depth = 2;
static bool show_hidden = false;
static bool list_dir(fs_node_t node) {
    if((node.hidden || node.name[0] == '.') && !show_hidden) return true;
    if(indent_level >= max_depth) return true;
    indent_level++;

    for(int i = 0; i < indent_level-1; i++)
        printf("|   ");
    printf("|---");
    printf("%s (%s)\n", node.name, node.isdir ? "directory" : "file");

    if(node.isdir && !strcmp(node.name, ".") && !strcmp(node.name, ".."))
        fs_list_dir(&node, list_dir);

    indent_level--;

    return true;
}

static void help() {
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("help echo ticks ls read cd mkdir rm touch write");
    }
    else {
        if(strcmp(current_token, "echo")) puts("echo <string>");
        else if(strcmp(current_token, "ticks")) puts("ticks <no arg>");
        else if(strcmp(current_token, "ls")) puts(
                "ls <args> <directory>\n"
                "available arg:\n"
                "    -a          show hidden\n"
                "    -d <num>    tree depth");
        else if(strcmp(current_token, "read")) puts("read <filename>");
        else if(strcmp(current_token, "cd")) puts("cd path/to/child/dir");
        else if(strcmp(current_token, "mkdir")) puts("mkdir path/to/dir1/dir2/dir3");
        else if(strcmp(current_token, "rm")) puts("rm dir/or/file");
        else if(strcmp(current_token, "touch")) puts("touch <filename>");
        else if(strcmp(current_token, "write")) puts("write <filename>");
    }
}

static void echo() {
    current_token += strlen(current_token) + 1;
    puts(current_token);
}

static void ticks() {
    printf("%d\n", timer_get_ticks());
}

static void ls() {
    if(!current_node.valid) {
        puts("no fs installed");
        return;
    }
    max_depth = 1;
    show_hidden = false;
    char* ls_name = NULL;

    current_token = strtok(NULL, " ");
    while(current_token != NULL) {
        if(strcmp(current_token, "-d")) {
            current_token = strtok(NULL, " ");
            if(current_token == NULL) {
                puts("not enough arguement");
                return;
            }
            max_depth = atoi(current_token);
        }
        else if(strcmp(current_token, "-a"))
            show_hidden = true;
        else
            ls_name = current_token;

        current_token = strtok(NULL, " ");
    }

    if(ls_name == NULL) {
        // list current dir
        fs_list_dir(&current_node, list_dir);
        return;
    }
    else {
        fs_node_t node = fs_find(&current_node, ls_name);
        if(!node.valid) {
            printf("directory '%s' not found\n", ls_name);
            return;
        }
        fs_list_dir(&node, list_dir);
    }
}

static void read() {
    if(!current_node.valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no file input");
        return;
    }

    fs_node_t node = fs_find(&current_node, current_token);
    if(!node.valid) {
        printf("file '%s' not found\n", current_token);
        return;
    }
    if(node.isdir) {
        puts("not a file");
        return;
    }

    FILE f = file_open(&node, FILE_READ);
    char chr;
    while(file_read(&f, (uint8_t*)(&chr), 1) != ERR_FS_EOF) {
        putchar(chr);
    }
}

static void cd() {
    if(!current_node.valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no path provided");
        return;
    }
    fs_node_t node = fs_find(&current_node, current_token);
    if(!node.valid) {
        printf("no such directory %s\n", current_token);
        return;
    }
    if(!node.isdir) {
        puts("not a directory");
        return;
    }
    current_node = node;
}

static void mkdir() {
    if(!current_node.valid) {
        puts("no fs installed");
        return;
    }

    char* dirname;
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }
    dirname = current_token;

    bool recursive = false;
    for(int i = 0; current_token[i] != '\0'; i++) {
        if(current_token[i] == '/') {
            recursive = true;
            current_token = strtok(current_token, "/");
            break;
        }
    }
    fs_node_t _curr_node = current_node;

    while(current_token != NULL) {
        if(recursive) {
            dirname = current_token;
            current_token = strtok(NULL, "/");
        }

        fs_node_t node = fs_find(&_curr_node, dirname);
        if(node.valid) {
            printf("a file or directory with name '%s' has already existed\n", dirname);
            return;
        }

        fs_node_t newdir = fs_mkdir(&_curr_node, dirname);
        if(!newdir.valid) {
            printf("failed to create '%s'\n", dirname);
            return;
        }
        if(!recursive) return;
        else _curr_node = newdir;
    }
}

static void rm() {
    if(!current_node.valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }
    FS_ERR err = fs_rm_recursive(&current_node, current_token);
    if(err == ERR_FS_NOT_FOUND) {
        puts("file or dir not found");
    }
}

static void touch() {
    if(!current_node.valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }
    fs_node_t node = fs_touch(&current_node, current_token);
    if(!node.valid) {
        printf("a file or directory with name '%s' has already existed (or out of memory? idk)\n", current_token);
    }
}

static void write() {
    if(!current_node.valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }
    fs_node_t node = fs_find(&current_node, current_token);
    if(!node.valid) {
        node = fs_touch(&current_node, current_token);
        if(!node.valid) {
            puts("cannot create file");
            return;
        }
    }

    FILE f = file_open(&node, FILE_WRITE);

    input_len = 0;
    puts("writing mode. press ESC to exit");

    while(current_key.keycode != 0) { // 0 is esc keycode
        while(key_handled) continue; // wait for new key
        key_handled = true;

        if(current_key.mapped == '\b') {
            if(input_len == 0) continue;
            tty_set_cursor(tty_get_cursor() - 1); // move back
            tty_print_char(' ', -1, 0, false); // delete printed char
            input_len--;
            continue;
        }

        putchar(current_key.mapped);
        input[input_len++] = current_key.mapped;
        if(current_key.mapped == '\n' || input_len >= 512) {
            file_write(&f, (uint8_t*)input, input_len);
            input_len = 0;
        }
    }

    if(input_len > 0)
        file_write(&f, (uint8_t*)input, input_len);
    file_close(&f);
}

static void process_prompt() {
    current_token = strtok(input, " ");

    if(strcmp(current_token, "help")) help();
    else if(strcmp(current_token, "echo")) echo();
    else if(strcmp(current_token, "ticks")) ticks();
    else if(strcmp(current_token, "ls")) ls();
    else if(strcmp(current_token, "read")) read();
    else if(strcmp(current_token, "cd")) cd();
    else if(strcmp(current_token, "mkdir")) mkdir();
    else if(strcmp(current_token, "rm")) rm();
    else if(strcmp(current_token, "touch")) touch();
    else if(strcmp(current_token, "write")) write();
    else if(input_len == 0); // just skip
    else puts("unknow command");
    printf("[kernel@kshell %s]$ ", current_node.name);
    input_len = 0;
    input[0] = '\0';
}

void shell_set_current_node(fs_node_t node) {
    current_node = node;
}
void shell_start() {
    install_tick_listener(tick_listener);
    install_key_listener(kbd_listener);
    puts("welcome to the shell");
    puts("please dont use `cd PATH` and PATH contain `..` dir");
    puts("type `help` t show all command. `help <command>` to see all available argument");
    printf("[kernel@kshell %s]$ ", current_node.name);

    while(true) {
        if(key_handled) continue; // wait for new key
        key_handled = true;

        if(current_key.mapped == '\b') {
            if(input_len == 0) continue;
            tty_set_cursor(tty_get_cursor() - 1); // move back
            tty_print_char(' ', -1, 0, false); // delete printed char
            input_len--;
            continue;
        }

        putchar(current_key.mapped);
        if(current_key.mapped != '\n') {
            input[input_len++] = current_key.mapped;
        }
        else {
            input[input_len] = '\0';
            process_prompt();
        }
    }
}
