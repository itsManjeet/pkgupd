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

#include "Execute.h"

#include <sstream>

Executor &Executor::start() {
    if (pipe(pipe_fd) == -1) {
        throw std::runtime_error("pipe creating failed");
    }
    if (pipe(err_pipe) == -1) {
        throw std::runtime_error("pipe creating failed");
    }

    pid = fork();
    if (pid == -1) {
        throw std::runtime_error("failed to fork new process");
    } else if (pid == 0) {
        close(pipe_fd[0]);
        close(err_pipe[0]);

        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);

        close(pipe_fd[1]);
        close(err_pipe[1]);

        if (path_) chdir(path_->c_str());
        clearenv();


        if (execve(args_.front().c_str(), (char *const *) args_.data(), (char *const *) environ_.data()) == -1) {
            throw std::runtime_error("failed to exec");
        }
        std::unreachable();
    } else {
        close(pipe_fd[1]);
        close(err_pipe[1]);
    }
    return *this;
}

int Executor::wait(std::ostream *out, std::ostream *err) {
    char buffer[BUFSIZ];
    ssize_t bytes_read;

    while ((bytes_read = read(pipe_fd[0], buffer, BUFSIZ)) > 0) {
        if (out) out->write(buffer, BUFSIZ);
    }
    // TODO: do this in parallel
    while ((bytes_read = read(err_pipe[0], buffer, BUFSIZ)) > 0) {
        if (err) err->write(buffer, BUFSIZ);
    }
    close(pipe_fd[0]);

    int status;
    waitpid(pid, &status, 0);

    return status;
}

int Executor::run() {
    start();
    return wait({});
}

std::tuple<int, std::string> Executor::output() {
    std::stringstream output, error;
    start();
    int status = wait(&output, &error);
    return {status, output.str() + " " + error.str()};
}

