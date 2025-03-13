#define ONLY_MOUNT_UMOUNT
#define ONLY_UMOUNT
#include "cmds.c"

int main(int argc, char **argv) {
	do_umount(argc, argv);
	return 0;
}
