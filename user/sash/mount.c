#define ONLY_MOUNT_UMOUNT
#define ONLY_MOUNT
#include "cmds.c"

int main(int argc, char **argv) {
	do_mount(argc, argv);
	return 0;
}
