#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/mman.h>

#include "common.h"

int setup_socket(int epollfd, struct sockaddr *saddr, uint8_t tobind)
{
	int sock, rc;

	sock = socket(saddr->sa_family, SOCK_STREAM, 0);
	if (sock == -1) {
		fprintf(stderr, "socket error %d\n", errno);
		return -1;;
	}

	if (saddr->sa_family == AF_UNIX) {
		rc = bind(sock, saddr, sizeof(struct sockaddr_un));
		if (!tobind)
			rc = connect(sock, saddr, sizeof(struct sockaddr_un));
	} else if (saddr->sa_family == AF_INET) {
		if (tobind)
			rc = bind(sock, saddr, sizeof(struct sockaddr_in));
		else
			rc = connect(sock, saddr, sizeof(struct sockaddr_in));
	} else {
		fprintf(stderr, "Invalid domain\n");
		goto err_set;
	}

	if (rc == -1) {
		perror("socket bind/connect");
		goto err_set;
	}

	if (tobind) {
		rc = listen(sock, MAX_EVENTS - 1);
		if (rc == -1) {
			perror("Socket listen");
			goto err_set;
		}
	}

	if (epollfd) {
		struct epoll_event ev;

		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = sock;
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
			perror("epoll_ctl: frontend socket");
			goto err_set;
		}
	}

	return sock;

err_set:
	close(sock);
	return -1;
}

ssize_t send_binaries(int socket, const char *binary, const char *bs, enum mnode_type msg_type, uint32_t id) {
	struct stat st1, st2;
	struct com_nod node_com = {0};

	// mapping binaries
	int fd1 = open(binary, O_RDONLY);
	if (fd1 < 0) {
		perror("Opening binary file to send");
		return -1;
	}
	int fd2 = open(bs, O_RDONLY);
	if (fd2 < 0) {
		perror("Opening bs file to send");
		return -1;
	}

	int rc = fstat(fd1, &st1);
	if (rc < 0) {
		perror("Getting binary file size");
		return -1;
	}
	rc = fstat(fd2, &st2);
	if (rc < 0) {
		perror("Getting bs file size");
		return -1;
	}

	void *new_addr1 = mmap(NULL, st1.st_size, PROT_READ, MAP_PRIVATE, fd1, 0);
	if (new_addr1 == MAP_FAILED) {
        perror("mmap failed\n");
        return -1;
    }

	void *new_addr2 = mmap(NULL, st2.st_size, PROT_READ, MAP_PRIVATE, fd2, 0);
	if (new_addr2 == MAP_FAILED) {
        perror("mmap failed\n");
        return -1;
    }

	close(fd1);
	close(fd2);


	// sending com_nod
	node_com.type = msg_type;
	node_com.tsk.size = st1.st_size;
	node_com.tsk.bs_size = st2.st_size;
	node_com.tsk.id = id;
	rc = write(socket, &node_com, sizeof(struct com_nod));
	if (rc < sizeof(struct com_nod)) {
		if (rc < 0)
			perror("Sending bin");
		else
			err_print("Short send of bin\n");
		return -1;
	}

	// sending a uk binary and a bitstream
	int res1 = write_with_check(socket, new_addr1, st1.st_size);
	int res2 = write_with_check(socket, new_addr2, st2.st_size);

	rc = munmap(new_addr1, st1.st_size);
	if (rc < 0) {
		perror("munmap");
		return -1;
	}
	rc = munmap(new_addr2, st2.st_size);
	if (rc < 0) {
		perror("munmap");
		return -1;
	}

	return res1 + res2;
}

ssize_t write_with_check(int socket, void* addr, off_t size) {
	int count = 0;
	while (count < size) {
		int n = write(socket, addr + count, size - count);
		if (n < 0) {
			perror("Writing file");
			return -1;
		}
		count += n;
	}

	if (count != size) {
		perror("Writing file, not match size\n");
		return -1;
	}
	return count;
}

/*
 * Send a file over a socket
 */
ssize_t send_file(int socket, const char *filename, enum mnode_type msg_type,
		  uint32_t id)
{
	int rc = 0, fd;
	struct stat st;
	ssize_t n, count = 0;
	struct com_nod node_com = {0};

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("Opening file to send");
		return -1;
	}

	/*
	 * Get size of file
	 */
	rc = stat(filename, &st);
	if (rc < 0) {
		perror("Getting file size");
		goto err_send;
	}

	node_com.type = msg_type;
	node_com.tsk.size = st.st_size;
	node_com.tsk.id = id;
	rc = write(socket, &node_com, sizeof(struct com_nod));
	if (rc < sizeof(struct com_nod)) {
		if (rc < 0)
			perror("Sending file info");
		else
			err_print("Short send of file info\n");
		goto err_send;
	}

	/*
	 * Make sure the whole file is sent.
	 */
	while (count < st.st_size) {
		n = sendfile(socket, fd, NULL, st.st_size - count);
		if (n < 0) {
			perror("Sending file");
			goto err_send;
		}
		count += n;
	}
	return count;

err_send:
	close(fd);
	return -1;
}
