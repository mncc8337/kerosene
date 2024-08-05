#include "kshell.h"
#include "kbd.h"
#include "timer.h"
#include "tty.h"
#include "filesystem.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

typedef enum {
    ERR_SHELL_NOT_FOUND,
    ERR_SHELL_SUCCESS,
    ERR_SHELL_TARGET_NOT_FOUND,
    ERR_SHELL_NOT_A_DIR
} SHELL_ERR;

static char input[512];
static unsigned int input_len = 0;
static char* current_token;

// a stack that contain the path of current dir
static fs_node_t node_stack[NODE_STACK_MAX_LENGTH];
static unsigned int node_stack_offset = 0;

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

    if(node.isdir) tty_set_attr(LIGHT_BLUE);
    puts(node.name);
    if(node.isdir) tty_set_attr(LIGHT_GREY);

    if(node.isdir && !strcmp(node.name, ".") && !strcmp(node.name, ".."))
        fs_list_dir(&node, list_dir);

    indent_level--;

    return true;
}
static int path_find_last_node(char* path, fs_node_t* parent, fs_node_t* node) {
    node->valid = false;

    char* nodename = strtok(path, "/");
    while(nodename != NULL) {
        if(node->valid) *parent = *node;
        *node = fs_find(parent, nodename);
        if(!node->valid) {
            memcpy(node->name, nodename, strlen(nodename)+1);
            if(strtok(NULL, "/") != NULL)
                return ERR_SHELL_NOT_FOUND;
            return ERR_SHELL_TARGET_NOT_FOUND;
        }
        nodename = strtok(NULL, "/");
        if(nodename != NULL && !node->isdir) // this is illegal
            return ERR_SHELL_NOT_A_DIR;
    }

    return ERR_SHELL_SUCCESS;
}

static void help() {
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("help .<n dot> echo ticks ls read cd mkdir rm touch write");
    }
    else {
        if(current_token[0] == '.') printf("go back %d dir\n", strlen(current_token)-1);
        else if(strcmp(current_token, "echo")) puts("echo <string>");
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
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }
    max_depth = 1;
    show_hidden = false;
    char* ls_name = NULL;

    current_token = strtok(NULL, " ");
    while(current_token != NULL) {
        if(current_token[0] == '-') {
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
            else {
                printf("unknown argument: %s\n", current_token);
                return;
            }
        }
        else
            ls_name = current_token;

        current_token = strtok(NULL, " ");
    }

    if(ls_name == NULL) {
        // list current dir
        fs_list_dir(current_node, list_dir);
        return;
    }
    else {
        fs_node_t node_parent = *current_node;
        fs_node_t node;
        SHELL_ERR err = path_find_last_node(ls_name, &node_parent, &node);
        if(err == ERR_SHELL_NOT_FOUND
                || err == ERR_SHELL_NOT_A_DIR
                || err == ERR_SHELL_TARGET_NOT_FOUND
                || !node.isdir) {
            printf("no such directory '%s'\n", node.name);
            return;
        }

        fs_list_dir(&node, list_dir);
    }
}

static void read() {
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no file input");
        return;
    }

    fs_node_t node_parent = *current_node;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(current_token, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND || node.isdir) {
        printf("no such file '%s'\n", node.name);
        return;
    }

    FILE f = file_open(&node, FILE_READ);
    char chr;
    while(file_read(&f, (uint8_t*)(&chr), 1) != ERR_FS_EOF) {
        putchar(chr);
    }
}

static void cd() {
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no path provided");
        return;
    }

    // now current token contain the path
    char* nodename = strtok(current_token, "/");
    while(nodename != NULL) {
        if(strcmp(nodename, "..")) node_stack_pop();
        else if(!strcmp(nodename, ".")) {
            fs_node_t tmp = fs_find(node_stack_top(), nodename);
            if(!tmp.valid) {
                printf("no such directory '%s'\n", nodename);
                return;
            }
            if(!tmp.isdir) {
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

static void mkdir() {
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }

    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }

    fs_node_t _curr_node = *current_node;

    char* dirname = strtok(current_token, "/");
    while(dirname != NULL) {
        fs_node_t node = fs_find(&_curr_node, dirname);
        if(node.valid) {
            printf("a file or directory with name '%s' has already existed\n", dirname);
            return;
        }

        fs_node_t newdir = fs_mkdir(&_curr_node, dirname);
        if(!newdir.valid) {
            printf("failed to create directory '%s'\n", dirname);
            return;
        }
        _curr_node = newdir;
        dirname = strtok(NULL, "/");
    }
}

static void rm() {
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }
    
    fs_node_t node_parent = *current_node;
    fs_node_t node;
    SHELL_ERR serr = path_find_last_node(current_token, &node_parent, &node);
    if(serr == ERR_SHELL_NOT_FOUND || serr == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(serr == ERR_SHELL_TARGET_NOT_FOUND) {
        printf("no such file or directory '%s'\n", node.name);
        return;
    }

    FS_ERR err = fs_rm_recursive(&node_parent, node);
    if(err != ERR_FS_SUCCESS)
        printf("cannot remove '%s'. error code %d\n", node.name, err);
}

static void touch() {
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }

    fs_node_t node_parent = *current_node;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(current_token, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("'%s' is not a directory\n", node.name);
        return;
    }
    if(err == ERR_SHELL_SUCCESS) {
        // we dont want it to find a valid node
        printf("a file or directory with name '%s' has already existed\n", current_token);
        return;
    }
    // at this point the error should be ERR_SHELL_TARGET_NOT_FOUND
    // which is what we wanted

    node = fs_touch(&node_parent, node.name);
    if(!node.valid) printf("cannot create '%s', out of space\n", node.name);
}

static void write() {
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }
    current_token = strtok(NULL, " ");
    if(current_token == NULL) {
        puts("no name provided");
        return;
    }

    fs_node_t node_parent = *current_node;
    fs_node_t node;
    SHELL_ERR err = path_find_last_node(current_token, &node_parent, &node);
    if(err == ERR_SHELL_NOT_FOUND || err == ERR_SHELL_NOT_A_DIR) {
        printf("no such directory '%s'\n", node.name);
        return;
    }
    if(err == ERR_SHELL_TARGET_NOT_FOUND) {
        // try to create one
        node = fs_touch(&node_parent, node.name);
        if(!node.valid) {
            printf("cannot create file '%s', out of space\n", node.name);
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

static void mv() {
    fs_node_t* current_node = node_stack_top();
    if(!current_node->valid) {
        puts("no fs installed");
        return;
    }
    char* source = strtok(NULL, " ");
    if(source == NULL) {
        puts("no source provided");
        return;
    }
    char* target = strtok(NULL, " ");
    if(target == NULL) {
        puts("no target provided");
        return;
    }
    puts("not implemented");
}

static void process_prompt() {
    current_token = strtok(input, " ");

    if(strcmp(current_token, "help")) help();
    else if(current_token[0] == '.') { // special command to go to parent dir
        unsigned int back_cnt = input_len - 1;
        if(back_cnt > node_stack_offset) node_stack_offset = 0;
        else node_stack_offset -= back_cnt;
    }
    else if(strcmp(current_token, "echo")) echo();
    else if(strcmp(current_token, "echo")) echo();
    else if(strcmp(current_token, "ticks")) ticks();
    else if(strcmp(current_token, "ls")) ls();
    else if(strcmp(current_token, "read")) read();
    else if(strcmp(current_token, "cd")) cd();
    else if(strcmp(current_token, "mkdir")) mkdir();
    else if(strcmp(current_token, "rm")) rm();
    else if(strcmp(current_token, "touch")) touch();
    else if(strcmp(current_token, "write")) write();
    else if(strcmp(current_token, "mv")) mv();
    else if(input_len == 0); // just skip
    else puts("unknow command");
    printf("[kernel@kshell %s ]$ ", node_stack[node_stack_offset].name);
    input_len = 0;
    input[0] = '\0';
}

void shell_set_root_node(fs_node_t node) {
    node_stack[0] = node;
    node_stack_offset = 0;
}
void shell_start() {
    install_tick_listener(tick_listener);
    install_key_listener(kbd_listener);
    puts("welcome to the shell");
    puts("type `help` t show all command. `help <command>` to see all available argument");
    printf("[kernel@kshell %s ]$ ", node_stack[node_stack_offset].name);

    while(true) {
        if(key_handled) continue; // wait for new key
        key_handled = true;

        // ignore non printable characters
        if(current_key.mapped == '\0') continue;

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
