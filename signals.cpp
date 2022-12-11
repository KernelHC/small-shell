#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/wait.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    int last_pid = getLastPid();
//    cout<<endl<<endl<<last_pid<<endl<<endl;
    cout << "smash: got ctrl-Z" << endl;
    if (checkIfContainAndStop(last_pid)) {
        return;
    }
    if (last_pid != -1) {
        if (waitpid(last_pid, nullptr, WNOHANG) == 0) {
            stopLastJob();
            updateLastPid(-1);

            if (kill(last_pid, SIGTSTP) == -1) {
                perror("smash error: kill failed");
                return;
            }
            cout << "smash: process " << last_pid << " was stopped" << endl;
        }
    }

}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    int last_pid = getLastPid();
    if (last_pid != -1) {
        if (jobIsStopped(last_pid))
            return;
        if (waitpid(last_pid, nullptr, WNOHANG) == 0) {
            updateLastPid(-1);
            if (kill(last_pid, SIGKILL) == -1) {
                perror("smash error: kill failed");
                return;
            }
            cout << "smash: process " << last_pid << " was killed" << endl;
        }
    }
}

void alarmHandler(int sig_num) {
    cout << "smash got an alarm" << endl;
}

