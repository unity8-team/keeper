
#include "mir-mock.h"

#include <iostream>
#include <thread>

#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

static const char * valid_trust_session = "In the circle of trust";
static bool valid_trust_connection = true;
static pid_t last_trust_pid = 0;
static int trusted_fd = 1234;

MirPromptSession *
mir_connection_create_prompt_session_sync(MirConnection * connection, pid_t pid, void (*)(MirPromptSession *, MirPromptSessionState, void*data), void * context) {
	last_trust_pid = pid;

	if (valid_trust_connection) {
		return (MirPromptSession *)valid_trust_session;
	} else {
		return nullptr;
	}
}

void
mir_prompt_session_release_sync (MirPromptSession * session)
{
	if (reinterpret_cast<char *>(session) != valid_trust_session) {
		std::cerr << "Releasing a Mir Trusted Prompt that isn't valid" << std::endl;
		exit(1);
	}
}

MirWaitHandle *
mir_prompt_session_new_fds_for_prompt_providers (MirPromptSession * session, unsigned int numfds, mir_client_fd_callback cb, void * data) {
	if (reinterpret_cast<char *>(session) != valid_trust_session) {
		std::cerr << "Releasing a Mir Trusted Prompt that isn't valid" << std::endl;
		exit(1);
	}

	/* TODO: Put in another thread to be more mir like */
	std::thread * thread = new std::thread([session, numfds, cb, data]() {
		int fdlist[numfds];

		for (unsigned int i = 0; i < numfds; i++) 
			fdlist[i] = trusted_fd;

		cb(session, numfds, fdlist, data);
	});

	return reinterpret_cast<MirWaitHandle *>(thread);
}

void
mir_wait_for (MirWaitHandle * wait)
{
	auto thread = reinterpret_cast<std::thread *>(wait);

	if (thread->joinable())
		thread->join();

	delete thread;
}

static const char * valid_connection_str = "Valid Mir Connection";
static std::pair<std::string, std::string> last_connection;
static bool valid_connection = true;

void
mir_mock_connect_return_valid (bool valid)
{
	valid_connection = valid;
}

std::pair<std::string, std::string>
mir_mock_connect_last_connect (void)
{
	return last_connection;
}

MirConnection *
mir_connect_sync (char const * server, char const * appname)
{
	last_connection = std::pair<std::string, std::string>(server, appname);

	if (valid_connection) {
		return (MirConnection *)(valid_connection_str);
	} else {
		return nullptr;
	}
}

void
mir_connection_release (MirConnection * con)
{
	if (reinterpret_cast<char *>(con) != valid_connection_str) {
		std::cerr << "Releasing a Mir Connection that isn't valid" << std::endl;
		exit(1);
	}
}

void mir_mock_set_trusted_fd (int fd)
{
	trusted_fd = fd;
}
