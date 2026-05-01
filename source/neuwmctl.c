#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK "/tmp/neuwm.sock"
#define BUF  4096

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: neuwmctl <cmd> [args...]\n");
		return 1;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return 1;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCK, sizeof(addr.sun_path) - 1);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(fd);
		return 1;
	}

	char cmd[BUF] = {0};
	for (int i = 1; i < argc; i++) {
		if (i > 1)
			strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
		strncat(cmd, argv[i], sizeof(cmd) - strlen(cmd) - 1);
	}
	strncat(cmd, "\n", sizeof(cmd) - strlen(cmd) - 1);

	size_t total = strlen(cmd), sent = 0;
	while (sent < total) {
		ssize_t n = write(fd, cmd + sent, total - sent);
		if (n < 0) {
			if (errno == EINTR) continue;
			perror("write");
			close(fd);
			return 1;
		}
		sent += n;
	}

	int is_sub = strcmp(argv[1], "sub") == 0;
	char buf[BUF];
	ssize_t n;

	while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
		buf[n] = '\0';
		char *msg = buf;
		if (!is_sub && strncmp(msg, "ok ", 3) == 0)
			msg += 3;
		printf("%s", msg);
		fflush(stdout);
		if (!is_sub)
			break;
	}

	close(fd);
	return 0;
}
