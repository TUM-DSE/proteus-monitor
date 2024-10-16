#ifndef SCHED_COMMON_H
#define SCHED_COMMON_H

#include <netinet/in.h>
#include <stdint.h>
#include <sys/types.h>

#define FRONT_SOCK	"/tmp/front.sock"
#define MAX_EVENTS	10
#define NODES_PORT	4217

#define err_print(fmt, ...)	fprintf(stderr, "%s - %d: " fmt, __func__, \
					__LINE__, ##__VA_ARGS__)

/*
 * Type of message between primary scheduler and daemons
 * It is mainly used to distinguish commands from files.
 */
enum mnode_type {
	deploy = 0,
	mig_cmd,
	evict,
	resume,
	migrate,
	arguments
};

/*
 * The state of a task. For the time being
 * only the first 2 states are used.
 */
enum task_state {
	ready = 0,
	running,
	stopped,
	done
};

struct task {
	uint32_t id;
	char *bin_path;
	char *bin_args; // Assumption: All bitstreams will have same args.
	struct bitstream *bitstreams;
	uint8_t num_bitstreams;
	uint8_t selected_bitstream; // Index of the currently selected bitstream from `bitstreams`
	uint8_t priority;
	enum task_state state;
	struct node *node;	// the node where the task has been deployed
	struct task *next;
	struct task *prev;
#ifdef TIME_TASK
	struct timespec tstart;
	long secs;
	long nsecs;
#endif
};

enum fpga_type {
	arria10,
	u50,
	u280,
	fpga_type_unsupported
};

static const char *fpga_type_to_str[] = {"arria10", "u50", "u280", "unsupported"};

struct bitstream {
	size_t size; // Size of bitstream in bytes
	uint32_t frequency; // Frequency of bitstream in Hz
	enum fpga_type fpga_type; // FPGA type the bitstream was compiled for
	char *file_path; // Path to the bitstream file
	char *data; // Pointer to raw bitstream data
};

/*
 * A struct which contains the result of a task execution
 * In case the task which completed was the evicted one then the
 * is_evicted value wil lbe set.
 */
struct tsk_res {
	uint8_t exit_code;
	uint32_t id;
};

struct tsk_dpl {
	off_t size;
	off_t bs_size;
	uint32_t id;
	enum fpga_type fpga_type;
};

/*
 * A struct which contains a message to a node.
 * This message precedes file transmission and in case of migration
 * notifies the daemon about the new node where task will migrate.
 */
struct com_nod {
	enum mnode_type type;
	union {
		struct tsk_dpl tsk;
		struct in_addr rcv_ip;
		size_t args_size;
	};
};

int setup_socket(int epollfd, struct sockaddr *saddr, uint8_t tobind);

/*
 * Send message of type `msg_type` containing binary and bitstream of `task` to `socket`.
 */
ssize_t send_binaries(int socket, enum mnode_type msg_type, const struct task *task);

ssize_t send_file(int socket, const char *filename, enum mnode_type msg_type, uint32_t id);

ssize_t write_with_check(int socket, void* addr, off_t size);

#endif /* SCHED_COMMON_H */
