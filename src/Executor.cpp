/* 
 * Copyright (c) 2023 Manjeet Singh <itsmanjeet1998@gmail.com>.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Executor.h"

#include <sstream>
#include <cstring>

Executor &Executor::start() {
    if (pipe(pipe_fd) == -1) {
        throw std::runtime_error("pipe creating failed");
    }

    pid = fork();
    if (pid == -1) {
        throw std::runtime_error("failed to fork new process");
    } else if (pid == 0) {
        close(pipe_fd[0]);

        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);

        close(pipe_fd[1]);

        if (path_) chdir(path_->c_str());
        clearenv();

        std::vector<const char *> args;
        for (auto const &c: args_) args.emplace_back(c.c_str());
        args.push_back(nullptr);

        std::vector<const char *> env;
        for (auto const &c: environ_) env.emplace_back(c.c_str());
        env.push_back(nullptr);

        if (execve(args_[0].c_str(), (char *const *) args.data(), (char *const *) env.data()) == -1) {
            throw std::runtime_error(strerror(errno));
        }
        std::unreachable();
    }
    return *this;
}

int Executor::wait(std::ostream *out) {
    close(pipe_fd[1]);

    char buffer[BUFSIZ] = {0};
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
        if (out) out->write(buffer, bytes_read);
    }

    int status;
    waitpid(pid, &status, 0);

    return WEXITSTATUS(status);
}

int Executor::run() {
    start();
    return wait({});
}

std::tuple<int, std::string> Executor::output() {
    std::stringstream output;
    start();
    int status = wait(&output);
    std::string output_data = output.str();
    output_data.shrink_to_fit();
    return {status, output_data};
}

