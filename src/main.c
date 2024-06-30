/*
 * Copyright (c) 2009-2012, Anthony Minessale II
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of the original author; nor the names of any contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Needed for asprintf()
#define _GNU_SOURCE

#include "esl/esl.h"
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// -----------------------------------------------------------------------------

static const char* s_short_help = "Usage: %s <script path> [script arguments...]";

static const char* s_long_help = "Description:\n\
	Start the IVR daemon, listening on <host>:<port>.\n\
\n\
Options:\n\
	-h, --help                 Display this help message.\n\
	-H, --hostname <hostname>  The hostname the daemon should listen to. Defaults to 127.0.0.1.\n\
	-p, --port <port>          The port the daemon should listen to. Defaults to 8084.\n\
	    --connect              Attach the ESL socket to a handle, effectively issuing a 'connect' command to FreeSwitch.\n\
	    --threaded             Use threads instead of fork()'ing to handle connections.\n\
	    --hotwire              Plug the connected socket's ends into a child process' STDIN/STDOUT.\n\
	                           Only when not --threaded.\n\
\n\
Arguments:\n\
	script path                Path to a program that will be executed to handle incoming connections.\n\
	                           It will receive the file descriptor of the connected socket on its arguments,\n\
	                           followed by the <script arguments> provided to the daemon.\n\
	script arguments           Any number of additional arguments to pass to the invoked <script path>.";

// -----------------------------------------------------------------------------

#define UNUSED __attribute__((unused))

typedef struct ivrd_options {
	const char* hostname;
	esl_port_t port;

	int threaded;
	int connect;
	int hotwire;

	const char* script_path;
	size_t script_argc;
	char* const* script_argv;
} ivrd_options_t;

// -----------------------------------------------------------------------------

static char** make_script_args(int client_sock, ivrd_options_t* options)
{
	// Compute the total size
	size_t size = 2 + options->script_argc;

	// Allocate the buffer
	char** vector = calloc(size + 1, sizeof(char*));
	if (vector == NULL) {
		return NULL;
	}

	// Fill the buffer
	// FIXME: NULL checks
	vector[0] = strdup(options->script_path);
	asprintf(&vector[1], "%d", client_sock);
	for (size_t i = 0; i < options->script_argc; ++i) {
		vector[i + 2] = strdup(options->script_argv[i]);
	}
	vector[size] = NULL;

	return vector;
}

static void free_args(char** args)
{
	for (char* arg = *args; arg != NULL; ++arg) {
		free(arg);
	}
	free(args);
}

// -----------------------------------------------------------------------------

static bool invoke_script(esl_socket_t client_sock, struct sockaddr_in* addr, ivrd_options_t* options)
{
	esl_handle_t handle = { 0 };

	if (options->connect) {
		// Connect to the ESL
		if (esl_attach_handle(&handle, client_sock, addr) != ESL_SUCCESS || !handle.info_event) {
			close(client_sock);
			fprintf(stderr, "Socket error\n");
			return false;
		}
	}

	// Invoke the script
	char** exec_args = make_script_args(client_sock, options);
	int r = execv(options->script_path, exec_args);

	// Cleanup
	free_args(exec_args);
	if (options->connect) {
		esl_disconnect(&handle);
	}

	if (r == -1) {
		fprintf(stderr, "execv() failed: %m\n");
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------

static void handle_SIGCHLD(UNUSED int sig)
{
	pid_t pid = -1;
	int status = 0;
	int saved_errno = errno;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		fprintf(stderr, "Child of PID %d exited with status %d\n", pid, status);
	}

	errno = saved_errno;
	return;
}

static void fork_callback(UNUSED esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in* addr, void* user_data)
{
	signal(SIGCHLD, handle_SIGCHLD);

	pid_t pid = fork();
	if (pid < 0) { // Failure
		fprintf(stderr, "Fork failed\n");
		close(client_sock);
		return;
	} else if (pid > 0) { // In the parent (ivrd)
		fprintf(stderr, "Forked to pid %d\n", pid);
		close(client_sock);
		return;
	}

	// In the child
	ivrd_options_t* options = (ivrd_options_t*)user_data;

	// Hotwire the socket to STDIN/STDOUT
	// This is actually not needed because the socket is accessible from the script
	if (options->hotwire) {
		if (dup2(client_sock, STDIN_FILENO) == -1 || dup2(client_sock, STDOUT_FILENO) == -1) {
			fprintf(stderr, "Failed to hotwire client socket to STDIN/STDOUT.\n");
			exit(EXIT_FAILURE);
		}
	}

	if (!invoke_script(client_sock, addr, user_data)) {
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

// -----------------------------------------------------------------------------

static void thread_callback(UNUSED esl_socket_t server_sock, esl_socket_t client_sock, struct sockaddr_in* addr, void* user_data)
{
	invoke_script(client_sock, addr, user_data);
}

// -----------------------------------------------------------------------------

static void print_usage(FILE* fp, const char* program, bool long_display)
{
	fprintf(fp, s_short_help, program);
	fputs("\n", fp);

	if (!long_display) {
		return;
	}

	fputs("\n", fp);
	fputs(s_long_help, fp);
	fputs("\n", fp);
}

static bool parse_options(ivrd_options_t* options, int argc, char* const argv[])
{
	static const char* short_opts = "hH:p:";
	const struct option long_opts[] = {
		{ "connect",  no_argument,       &options->connect,  1  },
		{ "help",     no_argument,       NULL,               'h'},
		{ "hostname", required_argument, NULL,               'H'},
		{ "hotwire",  no_argument,       &options->hotwire,  1  },
		{ "port",     required_argument, NULL,               'p'},
		{ "threaded", no_argument,       &options->threaded, 1  },
		{ NULL,       0,                 NULL,               0  }
	};

	for (;;) {
		int option_index = 0;
		int opt = getopt_long(argc, argv, short_opts, long_opts, &option_index);
		if (opt == -1) {
			break;
		}

		switch (opt) {
		case 0:
			break;
		case 'h':
			print_usage(stdout, argv[0], true);
			exit(EXIT_SUCCESS);
		case 'H':
			options->hostname = optarg;
			break;
		case 'p':
			options->port = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Unknown option: %c (%d)\n", opt, opt);
			return false;
		}
	}

	// Sanity checks
	if (options->threaded && options->hotwire) {
		fprintf(stderr, "Cannot use --hotwire and --threaded at the same time.\n");
		return false;
	}

	// The script path is a required argument
	if (argc - optind < 1) {
		fprintf(stderr, "The script path is missing.\n");
		return false;
	}

	options->script_path = argv[optind++];

	// The rest of the arguments will get passed to the invoked script.
	options->script_argc = argc - optind;
	options->script_argv = argv + optind;

	return true;
}

int main(int argc, char* const argv[])
{
	ivrd_options_t options = { 0 };
	options.hostname = "127.0.0.1";
	options.port = 8084;

	if (!parse_options(&options, argc, argv)) {
		print_usage(stderr, argv[0], false);
		return EXIT_FAILURE;
	}

	/**
	 * NOTE: fflush() stdout before entering esl_listen().
	 * Not doing so causes the fork()ed child to repeat the message for every incoming
	 * connection, if stdout has been redirected (into a logfile, for example).
	 */
	printf("Starting %s listener on %s:%d.\n", options.threaded ? "threaded" : "forking", options.hostname, options.port);
	fflush(stdout);

	if (options.threaded) {
		esl_listen_threaded(options.hostname, options.port, thread_callback, &options, 100000);
	} else {
		esl_listen(options.hostname, options.port, fork_callback, &options, NULL);
	}

	return EXIT_SUCCESS;
}
