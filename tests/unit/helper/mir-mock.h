
#ifndef MIR_MOCK_H
#define MIR_MOCK_H 1

#include <string>
#include <utility>

void mir_mock_connect_return_valid (bool valid);
std::pair<std::string, std::string> mir_mock_connect_last_connect (void);
void mir_mock_set_trusted_fd (int fd);

#endif // MIR_MOCK_H
