#include <stdio.h>
#include <time.h>

#include <syscall.h>

int buffer[512];
char sss[] = "ngu1";

int main() {
    int ret;

    printf("hi user eeee!\n");

    puts("buffer manipulation test");
    for(int i = 0; i < 512; i++)
        buffer[i] = i;

    puts("buffer print");
    for(int i = 0; i < 512; i++)
        printf("%d ", buffer[i]);
    putchar('\n');

    puts("raw print test");
    SYSCALL_3P(SYSCALL_WRITE, ret, 0, "\
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut porttitor magna et mauris iaculis scelerisque. Vivamus vel semper quam. Mauris pharetra accumsan magna et elementum. Duis egestas, urna non eleifend finibus, tortor lorem pretium lacus, id iaculis massa ante eget sem. Suspendisse porta velit dapibus, pellentesque ex eu, porta mi. Nullam et dignissim nibh. Morbi enim risus, molestie ac ligula nec, lobortis placerat lorem. Vivamus sit amet libero porttitor, mattis mi in, volutpat lacus. Nullam venenatis, quam ut imperdiet iaculis, sapien purus consectetur dolor, a euismod libero felis quis lacus.\n\n\
Proin luctus sodales mauris dictum tempor. Vestibulum quis pharetra lorem, id cursus libero. Vestibulum feugiat ante malesuada felis tincidunt, rutrum condimentum tortor lacinia. Phasellus vitae suscipit turpis, non tristique magna. Curabitur id sem suscipit, egestas est ac, blandit ante. Cras quis dolor ipsum. Nulla eget nisi ut nunc sagittis interdum. In vitae venenatis arcu. Ut sit amet mattis mauris. Curabitur ullamcorper eros eu nibh fermentum, at convallis mauris elementum. Mauris eu pulvinar nisi, eu volutpat lectus. Morbi vehicula tortor non enim dictum, vel lobortis nulla imperdiet. Morbi risus elit, elementum at bibendum vitae, mollis id eros.\n\n\
Quisque lacinia orci vitae diam tempus finibus. Curabitur tincidunt et diam vel tincidunt. Maecenas sagittis odio et libero eleifend lacinia. Morbi arcu arcu, ornare euismod eros eu, convallis facilisis elit. Morbi pretium quam in ante semper molestie. Morbi semper quam cursus, tincidunt lectus at, sodales ligula. Nam in feugiat velit.\n\n\
Nulla viverra fringilla mi accumsan malesuada. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Phasellus posuere quam et arcu tincidunt, volutpat ultricies lectus varius. Vestibulum venenatis sem et nisi pharetra congue. Nunc a odio accumsan, semper lorem sit amet, rutrum arcu. Maecenas porta, ligula ut scelerisque finibus, sapien odio tincidunt nisi, ac elementum lorem nisl vitae lacus. Praesent et est id lectus commodo mollis ac vitae lectus. Proin rutrum ornare nulla ac ultrices. Nam quis pharetra turpis. Morbi at tortor ut odio interdum pulvinar nec sed libero. Donec vel accumsan nulla. Donec purus leo, maximus vitae tellus vel, bibendum rutrum felis.\n\n\
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean vitae lorem mollis, facilisis justo vel, congue erat. Nunc dictum tempor ex, eu cursus quam sodales viverra. Mauris in urna vel nunc convallis luctus. In quis euismod elit, quis vestibulum massa. Duis sapien nisl, luctus eget leo vel, posuere eleifend velit. Praesent sed tincidunt dolor, id blandit tellus. Maecenas elementum tempus dapibus. Pellentesque convallis sed tellus et euismod. Proin malesuada augue sit amet nulla volutpat, ac porttitor mi maximus. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Quisque libero ex, sollicitudin et pretium nec, convallis sit amet odio. Aenean at justo tortor. Nullam felis magna, lacinia lacinia ligula vel, commodo scelerisque augue.", 3103);

    printf("print a string from memory: %s\n", sss);

    printf("current time: %d\n", time(NULL));

    SYSCALL_0P(SYSCALL_KILL_PROCESS, ret);
    return 0;
}
