#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <iomanip>
#include <utime.h>
#include "Commands.h" // todo moraja3et elarqam

using namespace std;
int max_jobID = 0;
int g_last_pid = -1;
string last_command("");
SmallShell &smash = SmallShell::getInstance();
char global_buff[200] = "";
bool piping = false;
#if 0
#define FUNC_ENTRY()
cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif
std::string WHITESPACE(" \t\f\v\n\r");

enum KILLRS {
    INVALID_ARGUMENTS, DOES_NOT_EXIST, SYSCALL_FAIL, KSUCCESS
};

bool checkIfContainAndStop(int pid) { // todo delete job
    smash.jobs_list->removeFinishedJobs();
    JobsList::JobEntry *job = smash.jobs_list->getJobByPid(pid);
    if (!job)
        return false;
    if (job->IS_STOPPED) {
        delete job;
        return true;
    }
    smash.jobs_list->convertToStopped(pid);

    delete job;
    return true;

}

bool jobIsStopped(int pid) {
    JobsList::JobEntry *job = smash.jobs_list->getJobByPid(pid);
    if (!job)
        return false;
    if (job->IS_STOPPED) {
        delete job;
        return true;
    }
    delete job;
    return false;
}

void updateLastPid(int pid) {
    g_last_pid = pid;
}

int getLastPid() {
    return g_last_pid;
}

bool isNumber(string str) {
    if ((str[0] == '-') && (str.length() > 1))
        str = str.substr(1);
    for (char const &c: str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

bool isValidSigNumber(string str) {
    if (!isNumber(str))
        return false;
    if ((str[0] == '-') && (str.length() > 1))
        str = str.substr(1);
    int num = stoi(str);
    if ((num < 0) || (num > 64))
        return false;
    return true;
}

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}


bool cdIsValid(vector<string> vec) {
    if ((vec[2] != "")) {
        cerr << "smash error: cd: too many arguments" << endl;
        return false;
    }
    if (vec[1] == "-") {
        if (smash.jobs_list->last_dir == "") {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return false;
        }
    }
    return true;
}


bool fgIsValid(vector<string> vec) {


    if ((vec[2] != "") || (!isNumber(vec[1]))) {//('vec[1]'>'9')&&('vec[1]')<'0')
        cerr << "smash error: fg: invalid arguments" << endl;
        return false;
    }
    if (smash.jobs_list->all_jobs.empty()) {
        cerr << "smash error: fg: jobs list is empty" << endl;
        return false;
    }
    return true;
}

bool bgIsValid(vector<string> vec) {

    if ((vec[2] != "") || (!isNumber(vec[1]))) {//('vec[1]'>'9')&&('vec[1]')<'0')
        cerr << "smash error: bg: invalid arguments" << endl;
        return false;
    }
    if (smash.jobs_list->stopped_jobs.empty()) {
        cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
        return false;
    }
    return true;
}

int bgGetJobID(vector<string> vec) {
    int max = 0;
    for (unsigned int i = 0; i < smash.jobs_list->all_jobs.size(); i++) {
        if ((smash.jobs_list->all_jobs[i].jobID > max) && (smash.jobs_list->all_jobs[i].IS_STOPPED))
            max = smash.jobs_list->all_jobs[i].jobID;
    }
    if (vec[1] == "") {
        return max;
    }
    return atoi(vec[1].c_str());
}

int fgGetJobID(vector<string> vec) {
    int max = 0;
    for (unsigned int i = 0; i < smash.jobs_list->all_jobs.size(); i++) {
        if (smash.jobs_list->all_jobs[i].jobID > max)
            max = smash.jobs_list->all_jobs[i].jobID;
    }
    if (vec[1] == "") {
        return max;
    }
    return atoi(vec[1].c_str());
}


void stopLastJob() {
//    char x[last_command.length() + 1];
//    strcpy(x,last_command.c_str());
//    BuiltInCommand*  comm = new (last_command.c_str());
    smash.jobs_list->addJob(last_command, g_last_pid, true);
}

//KILLRS KillChecker(string cmd_line) {
//    vector<string> args = splitS(cmd_line);
//    if((args[1] == "")||(args[2]=="")||(args[3] != ""))
//        return INVALID_ARGUMENTS;
//    string first_arg = args[1];
//    if((first_arg[0] != '-')||(first_arg.length()==1))
//        return INVALID_ARGUMENTS;
//    first_arg = first_arg.substr(1);
//    if((!isNumber(first_arg))||(!isNumber(args[2])))
//        return INVALID_ARGUMENTS;
//    if(atoi((args[2]).c_str())>99)
//        return DOES_NOT_EXIST;
//    if(!jobs_list->used_id[atoi((args[2]).c_str())])
//        return DOES_NOT_EXIST;
//    return KSUCCESS;
//};


int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

vector<string> splitS(string s1) {
    vector<string> vec(20, "");
    char *args[20];
    int size = _parseCommandLine(s1.c_str(), args);
    for (int i = 0; i < size; ++i) {
        vec[i] = args[i];
    }
    return vec;
}


bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool areSameCommands(const char *cmd1, const char *cmd2) {
    char x[strlen(cmd1) + 1];
    char y[strlen(cmd2) + 1];
    strcpy(x, cmd1);
    strcpy(y, cmd2);;
    _removeBackgroundSign(x);
    _removeBackgroundSign(y);
    vector<string> vec1 = splitS(x);
    vector<string> vec2 = splitS(y);
    for (int i = 0; i < 20; ++i) {
        if (vec1[i] != vec2[i])
            return false;
    }
    return true;
}

int isPipe(vector<string> vec) {
    for (int i = 1; i < 19; ++i) {
        if ((vec[i] == "|") || (vec[i] == "|&")) {
            return i;
//            if (vec[i + 1] != "") {
//                return i;
//            } else {
//                return -1;
//            }
        }
    }
    return -1;
}

int isRedir(vector<string> vec) {
    for (int i = 0; i < 19; ++i) {
        if ((vec[i] == ">") || (vec[i] == ">>")) {
            return i;
        }
    }
    return -1;
}
// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
    jobs_list = new JobsList();
}

//SmallShell::~SmallShell() {
//
//    cout << "destructor" << endl;
//    delete jobs_list;
//
//}
string firstWordRemoveUmprsnt(string s) {
    int len = s.length();
    if (len == 0)
        return "";
    if (s[len - 1] == '&')
        return s.substr(0, len - 1);
    return s;
}

bool bdl_used_id(int job_id, vector<JobsList::JobEntry> vec) {
    for (unsigned int i = 0; i < vec.size(); i++) {
        if (vec[i].jobID == job_id)
            return true;
    }
    return false;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));
    firstWord = firstWordRemoveUmprsnt(firstWord);
    jobs_list->removeFinishedJobs();
//todo pwd& ......
    vector<string> vec = splitS(cmd_line);
    g_last_pid = -1;
    int res = isPipe(vec);
    if (res != -1)
        return new PipeCommand(cmd_line, res);
    res = isRedir(vec);
    if (res != -1)
        return new RedirectionCommand(cmd_line, res);
    if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);

    } else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);

    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, nullptr);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, nullptr);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, nullptr);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, nullptr);
    } else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, nullptr);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, nullptr);
    } else if (firstWord.compare("tail") == 0) {
        return new TailCommand(cmd_line);
    } else if (firstWord.compare("touch") == 0) {
        return new TouchCommand(cmd_line);
    } else if (firstWord.compare("fare") == 0) {
        return new FareCommand(cmd_line);
    }
    return new ExternalCommand(cmd_line);


    return nullptr;
}


void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    Command *cmd = CreateCommand(cmd_line);
    if (!cmd)
        return;
    cmd->execute();
    delete cmd;
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

//////////////////////////////// CLASS FUNCS ///////////////////////////////
//Command::Command(const char *cmd_line) : cmd_line(cmd_line) {}

BuiltInCommand::BuiltInCommand(
        const char *cmd_line) : Command(cmd_line) {}

GetCurrDirCommand::GetCurrDirCommand(
        const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char buf[80];
    getcwd(buf, 80);
    cout << string(buf) << endl;
}

ShowPidCommand::ShowPidCommand(
        const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << smash.pid << endl;
}

ChangeDirCommand::ChangeDirCommand(
        const char *cmd_line,
        char **plastPwd) : BuiltInCommand(cmd_line),
                           plastPwd(plastPwd) {
}

void ChangeDirCommand::execute() {
    JobsList *joblst = smash.jobs_list;
    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    vector<string> vec = splitS(string(comm));
    if (!cdIsValid(vec))
        return;
    if (joblst->current_dir == "") {
        char buf[80];
        getcwd(buf, 80);
        joblst->current_dir = buf;
    }
    if (vec[1] == "-") {
        if (chdir(joblst->last_dir.c_str()) >= 0) {
            std::string temp = joblst->last_dir;
            joblst->last_dir = joblst->current_dir;
            joblst->current_dir = temp;
        } else {
            perror("smash error: chdir failed"); //
        }
    } else {
        if (chdir((vec[1]).c_str()) < 0) {
            perror("smash error: chdir failed"); //
            return;
        }
        char buf[80];
        getcwd(buf, 80);
        joblst->last_dir = joblst->current_dir;
        joblst->current_dir = buf;
    }

}

void JobsList::updateJobTime(int jobID) {
    int size = all_jobs.size();
    for (int i = 0; i < size; ++i) {
        if ((all_jobs[i]).jobID == jobID) {
            (all_jobs[i]).time0 = time(nullptr);
            return;
        }
    }
}

int getToAddJobID() {
    int max = 1;
    int size = smash.jobs_list->all_jobs.size();
    JobsList *lst = smash.jobs_list;
    for (int i = 0; i < size; ++i) {
        if (lst->all_jobs[i].jobID >= max) {
            max = lst->all_jobs[i].jobID + 1;
        }
    }
    return max;
}

void JobsList::addJob(string cmd_l, int pid, bool isStopped) {//todo tsle7 aresamecommand + mkre sleep100& = sleep 100 &
    time_t t = time(nullptr);
//    int size;
    max_jobID = getToAddJobID();
    removeFinishedJobs();
//    std::string cmd_l = cmd->cmd_line;
    JobEntry to_add(cmd_l, t, pid, max_jobID, isStopped);
    // used_id[max_jobID] = true;
    all_jobs.push_back(to_add);
    if (isStopped) {
//        size = stopped_jobs.size();
//        for (int i = 0; i < size; ++i) {
//            if (areSameCommands((stopped_jobs[i]).cmd_line.c_str(), cmd_l.c_str())) {
//                stopped_jobs[i].time0 = t;
//                all_jobs.pop_back();
//                updateJobTime(stopped_jobs[i].jobID);
//                return;
//            }
//        }
        this->stopped_jobs.push_back(to_add);
    } else {
//        size = jobs.size();
//        for (int i = 0; i < size; ++i) {
//            if (areSameCommands((jobs[i]).cmd_line.c_str(), cmd_l.c_str())) { // todo sleep 100 = sleep    100
//                jobs[i].time0 = t;
//                all_jobs.pop_back();
//                updateJobTime(jobs[i].jobID);
//                return;
//            }
//        }
        this->jobs.push_back(to_add);
    }

}

void JobsList::printJobsList() {

    int size = all_jobs.size();
    for (int i = 0; i < size; ++i) {
        JobEntry j = all_jobs[i];
        cout << "[" << j.jobID << "] " << j.cmd_line << " : " << j.pid << " " << difftime(time(nullptr), j.time0)
             << " secs";
        if (j.IS_STOPPED) {
            cout << " (stopped)";
        }
        cout << endl;
    }

}

void JobsList::printJobsListQuit() {

    int size = all_jobs.size();
    for (int i = 0; i < size; ++i) {
        JobEntry j = all_jobs[i];
        cout << j.pid << ": " << j.cmd_line;
//        if (j.IS_STOPPED) {
//            cout << " (stopped)";
//        }
        cout << endl;
    }

}

void JobsList::removeFinishedJobs() { // todo : remove used_id
    int all_size = all_jobs.size();
    int s_size = stopped_jobs.size();
    int u_size = jobs.size();
    vector<JobEntry> helper;
    for (int i = 0; i < all_size; ++i) {
        if (!(waitpid(all_jobs[i].pid, nullptr, WNOHANG))) {
            helper.push_back(all_jobs[i]);
        }
//        else {
//            used_id[all_jobs[i].jobID] = false;
//        }
    }
    all_jobs = helper;
    vector<JobEntry> helper2;
    for (int i = 0; i < s_size; ++i) {
        if (!(waitpid(stopped_jobs[i].pid, nullptr, WNOHANG)))
            helper2.push_back(stopped_jobs[i]);
    }
    stopped_jobs = helper2;
    vector<JobEntry> helper3;
    for (int i = 0; i < u_size; ++i) {
        if (!(waitpid(jobs[i].pid, nullptr, WNOHANG)))
            helper3.push_back(jobs[i]);
    }
    jobs = helper3;

}

void JobsList::killAllJobs() {
    std::vector<JobEntry> empty;
    all_jobs = empty;
    jobs = empty;
    stopped_jobs = empty;
//    for (int i = 0; i < 100; i++)
//        used_id[i] = false;
    max_jobID = 0;
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    int size = all_jobs.size();
    for (int i = 0; i < size; ++i) {
        if ((all_jobs[i]).jobID == jobId)
            return new JobEntry(all_jobs[i]);
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    int all_size = all_jobs.size();
    int u_size = jobs.size();
    int s_size = stopped_jobs.size();
//    if (!used_id[jobId])
//        return;
    if (!bdl_used_id(jobId, all_jobs))
        return;
    // used_id[jobId] = false;
    vector<JobEntry> helper;
    vector<JobEntry> helper1;
    vector<JobEntry> helper2;

    for (int i = 0; i < all_size; ++i) {
        if (all_jobs[i].jobID != jobId)
            helper.push_back(all_jobs[i]);
    }
    all_jobs = helper;
    for (int i = 0; i < u_size; ++i) {
        if (jobs[i].jobID != jobId)
            helper1.push_back(all_jobs[i]);
    }
    jobs = helper1;


    for (int i = 0; i < s_size; ++i) {
        if (stopped_jobs[i].jobID != jobId)
            helper2.push_back(stopped_jobs[i]);
    }
    stopped_jobs = helper2;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    int size = jobs.size();
    if (size >= 1)
        return new JobEntry(jobs[size - 1]);
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    int size = stopped_jobs.size();
    if (size >= 1)
        return new JobEntry(stopped_jobs[size - 1]);
    return nullptr;
}

void JobsList::sendSignalToJobs(int SIG) {
    int s_size = stopped_jobs.size();
    int size = jobs.size();
    for (int i = 0; i < s_size; ++i) {
        if (kill(stopped_jobs[i].pid, SIG) == -1)
            perror("smash error: kill failed");
    }
    for (int i = 0; i < size; ++i) {
        if (kill(jobs[i].pid, SIG) == -1)
            perror("smash error: kill failed");
    }
}

void JobsList::convertToUnstopped(int jobID) {
    int size = all_jobs.size();
    for (int i = 0; i < size; ++i) {
        if (all_jobs[i].jobID == jobID) {
            all_jobs[i].IS_STOPPED = false;
            if (kill(all_jobs[i].pid, SIGCONT) == -1) {
                perror("smash error: kill failed");
                return;
            }
            break;
        }
    }
    size = stopped_jobs.size();
    vector<JobEntry> helper;

    for (int i = 0; i < size; ++i) {
        if (stopped_jobs[i].jobID == jobID) {
            JobEntry to_change = stopped_jobs[i];
            to_change.IS_STOPPED = false;
            jobs.push_back(to_change);
        } else {
            helper.push_back(stopped_jobs[i]);
        }
    }
    stopped_jobs = helper;
}

void JobsList::listUndo() {
    int size = all_jobs.size();
    if (all_jobs[size - 1].IS_STOPPED) {
        stopped_jobs.pop_back();
    } else {
        jobs.pop_back();
    }
    all_jobs.pop_back();
}

void JobsList::convertToStopped(int pid) {
    int size = all_jobs.size();
    for (int i = 0; i < size; ++i) {
        if (all_jobs[i].pid == pid) {
            if (kill(all_jobs[i].pid, SIGTSTP) == -1) {
                perror("smash error: kill failed");
                return;
            }
            all_jobs[i].IS_STOPPED = true;
            break;
        }
    }
    size = jobs.size();
    vector<JobEntry> helper;

    for (int i = 0; i < size; ++i) {
        if (jobs[i].pid == pid) {
            JobEntry to_change = jobs[i];
            to_change.IS_STOPPED = true;
            stopped_jobs.push_back(to_change);
        } else {
            helper.push_back(jobs[i]);
        }
    }
    jobs = helper;
    cout << "smash: process " << pid << " was stopped" << endl;
}

JobsList::JobEntry *JobsList::getJobByPid(int pid) {
    int size = all_jobs.size();
    for (int i = 0; i < size; ++i) {
        if ((all_jobs[i]).pid == pid)
            return new JobEntry(all_jobs[i]);
    }
    return nullptr;
}

void JobsCommand::execute() {

    smash.jobs_list->printJobsList();
}

void KillCommand::execute() {
    //KILLRS res =KillChecker(string (cmd_line));
//    if(res == INVALID_ARGUMENTS){
//        perror("smash error: kill: invalid arguments")
//    }

    vector<string> args = splitS(cmd_line);
    if ((args[1] == "") || (args[2] == "") || (args[3] != "")) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    string first_arg = args[1];
    if ((first_arg[0] != '-') || (first_arg.length() == 1)) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    first_arg = first_arg.substr(1);
    if ((!isNumber(first_arg)) || (!isNumber(args[2])) || (!isValidSigNumber(first_arg))) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int jobid = atoi((args[2]).c_str());
//    if (jobid > 99) {
//        cout << "smash error: kill: job-id " << jobid << " does not exist" << endl;
//        return;
//    } // todo check the print way
//    if (!smash.jobs_list->used_id[jobid]) {
//        cout << "smash error: kill: job-id " << jobid << " does not exist" << endl;
//        return;
//    }
    if (!bdl_used_id(jobid, smash.jobs_list->all_jobs)) {
        cerr << "smash error: kill: job-id " << jobid << " does not exist" << endl;
        return;
    }
    int sig = atoi(first_arg.c_str());
    JobsList::JobEntry *job = smash.jobs_list->getJobById(jobid);
    int pid = job->pid;
    string helper = cmd_line;
    delete job;
    if (kill(pid, sig) == -1) {
        perror("smash error: kill failed");
        return;
    } else {

        cout << "signal number " << sig << " was sent to pid " << pid << endl;

    }
//    if (sig == SIGCONT) { todo ef7s eza hay ell if lasme wlla laa
//        smash.jobs_list->removeJobById(jobid);
//        last_command = helper;
//        g_last_pid = pid;
//    }

}

void ForegroundCommand::execute() {
    char comm[strlen(cmd_line + 1)];
    strcpy(comm, cmd_line);
    _removeBackgroundSign(comm);
    smash.jobs_list->removeFinishedJobs();
    vector<string> vec = splitS(comm);
    if (!fgIsValid(vec))
        return;
    int jobId = fgGetJobID(vec);
    JobsList::JobEntry *job = smash.jobs_list->getJobById(jobId);
    if (!job) {
        cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
        return;
    }
    if (job->IS_STOPPED) {
        smash.jobs_list->convertToUnstopped(jobId);
        if (kill(job->pid, SIGCONT) == -1) {
            perror("smash error: kill failed");
            return;
        }
    }
    g_last_pid = job->pid;
    last_command = job->cmd_line;
    cout << job->cmd_line << " : " << job->pid << endl;
    waitpid(job->pid, nullptr, WUNTRACED);
    delete job;
}

void QuitCommand::execute() {
    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    smash.jobs_list->removeFinishedJobs();
    vector<string> vec = splitS(comm);
    if (vec[1] == "kill") {
        smash.jobs_list->sendSignalToJobs(SIGKILL);
        int jobs_num = smash.jobs_list->stopped_jobs.size() + smash.jobs_list->jobs.size();
        cout << "smash: sending SIGKILL signal to " << jobs_num << " jobs:" << endl;
        smash.jobs_list->printJobsListQuit();
    }
}

void BackgroundCommand::execute() {

    smash.jobs_list->removeFinishedJobs();
    char comm[strlen(this->cmd_line + 1)];
    strcpy(comm, this->cmd_line);
    _removeBackgroundSign(comm);
    vector<string> vec = splitS(comm);
    if (!bgIsValid(vec))
        return;
    int jobId = bgGetJobID(vec);
    JobsList::JobEntry *job = smash.jobs_list->getJobById(jobId);
    if (!job) {
        cerr << "smash error: bg: job-id " << jobId << " does not exist" << endl;
        return;
    }
    if (!job->IS_STOPPED) {
//        cout <<endl<<endl<<jobId<<endl<<endl;
        cerr << "smash error: bg: job-id " << jobId << " is already running in the background" << endl;
        return;
    }
    if (kill(job->pid, SIGCONT) == -1) {
        perror("smash error: kill failed");
        return;
    }
    cout << job->cmd_line << " : " << job->pid << endl;
    smash.jobs_list->convertToUnstopped(jobId);
    delete job;
}


bool isSimple (vector<string>& vec) {
    for (string s : vec) {
        if (s.find('*') != string::npos || s.find('?') != string::npos)
            return false;
    }
    return true;
}

bool isExecutable (vector<string>& vec) {
    if ((vec[0][0] == '.' && vec[0][1] == '/') || !access(vec[0].c_str(), X_OK))
        return true;
    return false;
}

void simpleCommandRemoveBG (vector<string>& vec) {
    for (string& s : vec) {
        for (int i = 0; i < s.size(); i++) {
            if (s[i] == '&') {
                s.erase(i,1);
            }
        }
    }
}

int getArgsNum (vector<string>& vec) {
    int args_num = 0;
    for (int i = 0; i < vec.size(); i++) {
        if (vec[i] == "") return args_num;
        args_num++;
    }
    return args_num;
}

void ExternalCommand::execute() {// todo zbt el echo bl c
    // test
    vector<string> vec = splitS(cmd_line);
    bool umprsnt_flag = _isBackgroundComamnd(cmd_line);
    int pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (pid == 0) {
        setpgrp();
        char arg3[strlen(cmd_line) + 1];
        strcpy(arg3, cmd_line);
        _removeBackgroundSign(arg3);


        //simple/complex command update:
        if (isSimple(vec)) { // if it's an executable run directly
            simpleCommandRemoveBG(vec);
            int args_num = getArgsNum(vec);
            char* args[20] = {NULL};
            if (!isExecutable(vec)) {
                vec[0] = "/bin/" + vec[0];
            }
            for (int i = 0; i < args_num; i++) {
                if (vec[i] == "") break;
                args[i] = (char*)vec[i].c_str();
            }

            if (execv((char *) args[0], args) == -1)
                perror("smash error: execv failed");
            exit(0);
        }  // else run through bash
        else {
            char *args[] = {(char *) "/bin/bash", (char *) "-c", arg3, nullptr};
            if (execv((char *) "/bin/bash", args) == -1)
                perror("smash error: execv failed");
            exit(0);
        }
    }

    else {
        if (umprsnt_flag)
            smash.jobs_list->addJob(cmd_line, pid);
        if (!umprsnt_flag) {
            last_command = cmd_line;
            g_last_pid = pid;
            waitpid(pid, nullptr, WUNTRACED);
        }

    }
}

int buffSize(char *buff) {
    for (int i = 0; i < 200; ++i) {
        if (buff[i] == '\0')
            return i + 1;
    }
    return 0; // we don't get here !
}

string makeSecSpecialComand(const char *cmd_line, int i) {
    vector<string> vec = splitS(cmd_line);
    string to_ret = "";
    i++;

    for (; ((i < 20) && (vec[i] != "")); ++i) {
        //cout <<"vec[i] : " << vec[i] << " ";
        to_ret += vec[i];
        to_ret += " ";
    }
    int len = to_ret.length();
    if (len > 0)
        to_ret = to_ret.substr(0, len - 1);
    return to_ret;
}

int redirectionFirstOf(string sentence, string find) {
    int len = sentence.length();
    for (int i = 0; i < len; ++i) {
        if (sentence[i] == '>') {
            if (find == ">") {
                return i;
            } else if (sentence[i + 1] == '>') {
                return i + 1;
            }
        }
    }
    return -1;
}

string makeSecFile(const char *cmd_line, int index) {
    string to_ret = string(cmd_line);
    vector<string> vec = splitS(cmd_line);
    int j;
    if (vec[index] == ">")
        j = redirectionFirstOf(to_ret, ">");
    else
        j = redirectionFirstOf(to_ret, ">>");
    j++;

    to_ret = to_ret.substr(j);
    to_ret = _trim(to_ret);
    return to_ret;
}

string makeFirstSpecialCommand(const char *cmd_line, int index) {
    string s = "";
    if (!index)
        return " ";
    vector<string> vec = splitS(cmd_line);
    for (int i = 0; i < index; i++) {
        s += vec[i];
        s += " ";
    }
    s = s.substr(0, s.length() - 1);
    return s;
}

void PipeCommand::execute() {
    vector<string> vec = splitS(cmd_line);
    int my_pipe[2];
    pipe(my_pipe);
    int pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    int os = 1;
    if (pid == 0) {
        if (vec[index] == "|&")
            os = 2;
        setpgrp();
        close(my_pipe[0]);
        int helper = dup(os);
        dup2(my_pipe[1], os);
        if (index) {
            string helper = makeFirstSpecialCommand(cmd_line, index);
            char comm[helper.length() + 1];
            strcpy(comm, helper.c_str());
            smash.executeCommand(comm);
        } else { smash.executeCommand(""); }
        dup2(os, helper);
        close(helper);
        close(my_pipe[1]);
        exit(0);
    } else { // father
        int pid1 = fork();
        if (pid1 == -1) {
            perror("smash error: fork failed");
            return;
        }
        if (pid1 == 0) {
            setpgrp();
            //waitpid(pid, nullptr, WUNTRACED);
            close(my_pipe[1]);
            int helper1 = dup(0);
            dup2(my_pipe[0], 0);
            string s = makeSecSpecialComand(cmd_line, index);
            smash.executeCommand(s.c_str());
            dup2(0, helper1);
            close(my_pipe[0]);
            close(helper1);
            exit(0);
        } else {
            close(my_pipe[0]);
            close(my_pipe[1]);

            waitpid(pid, nullptr, WUNTRACED);
            waitpid(pid1, nullptr, WUNTRACED);
            kill(pid,SIGKILL);
            kill(pid1,SIGKILL);
        }
    }

}


void RedirectionCommand::execute() {
    vector<std::string> vec = splitS(cmd_line);
    string helper = makeSecFile(cmd_line, index);
    char file_name[helper.length() + 1];
    strcpy(file_name, helper.c_str());
//    if (std::string(file_name) == "") {
//        cout << " > >> error" << endl;
//        return;

//    }
    int to_change;
    if (vec[index] == ">") {
        to_change = open(file_name, O_TRUNC | O_WRONLY | O_CREAT, 0655);
        if (to_change == -1) {
            perror("smash error: open failed");
            return;
        }
    } else {
        to_change = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0655);
        if (to_change == -1) {
            perror("smash error: open failed");
            return;
        }
    }
    if (index) {
        int helper = dup(1);
        dup2(to_change, 1);
        string s = makeFirstSpecialCommand(cmd_line, index);
        char comm[s.length() + 1];
        strcpy(comm, s.c_str());
        smash.executeCommand(comm);
        dup2(helper, 1);
        close(helper);
    }
    close(to_change);
}

bool linesNumIsValid(vector<string> vec) {
    if ((vec[3] == "") && (vec[2] != "")) {
        return (vec[1][0] == '-');
    }
    return true;
}

bool tailCommandIsValid(vector<string> vec) {
    if ((vec[1] == "") || (vec[3] != "") || !(linesNumIsValid(vec))) {
        cerr << "smash error: tail: invalid arguments" << endl;
        return false;
    }
    return true;
}

int getLinesNum(vector<string> vec) {
    string s = vec[1];
    if (s[0] == '-') {
        s = s.substr(1);
        return (atoi(s.c_str()));
    }
    return 10;
}

string tailCommGetFileName(vector<string> vec) {
    string s = vec[1];
    if (s[0] == '-') {
        return vec[2];
    }
    return vec[1];
}

void TailCommand::execute() {
    vector<string> vec = splitS(cmd_line);
    if (!tailCommandIsValid(vec))
        return;
    int lines_num = getLinesNum(vec);
    if (lines_num == 0)
        return;
    int res;
    int all_lines_num = 0;
    string file_name = tailCommGetFileName(vec);
    int my_f0 = open(file_name.c_str(), O_RDWR, 0664);
    if (my_f0 == -1) {
        perror("smash error: open failed");
        return;
    }
    string s = "";
    char x;
    while ((res = read(my_f0, &x, 1) != 0)) {
        if (res == -1) {
            perror("smash error: read failed");
            return;
        }
        s += x;
        if (x == '\n')
            all_lines_num++;
    }
    close(my_f0);

    while ((all_lines_num != 0) && (s[s.size() - 1] == '\n')) {
        all_lines_num--;
        s = s.substr(0, s.size()-1);
    }
    if (all_lines_num == 0)
        return;

    s = "";
    int my_f = open(file_name.c_str(), O_RDWR, 0664);
    if (my_f == -1) {
        perror("smash error: open failed");
        return;
    }

    while ((res = read(my_f, &x, 1) != 0) && (all_lines_num >= lines_num)) {
        if (res == -1) {
            perror("smash error: read failed");
            return;
        }
        if (x == '\n')
            all_lines_num--;
    }
    if (lines_num >= all_lines_num)
        s += x;

    res = read(my_f, &x, 1);
    if (res == -1) {
        perror("smash error: read failed");
        return;
    }
    if (!res)
        return;
    if (lines_num >= all_lines_num)
        s += x;
    while ((res = read(my_f, &x, 1) != 0)) {
        if (res == -1) {
            perror("smash error: read failed");
            return;
        }
        s += x;
    }
    close(my_f);
    cout << s;
}

bool touchCommandIsValid(vector<string> vec) {
    if ((vec[1] == "") || (vec[2] == "")) {
        cerr << "smash error: touch: invalid arguments" << endl;
        return false;
    }
    return true;
}

string touchCommGetFileName(vector<string> vec) {
    return vec[1];
}

string touchCommGetTime(vector<string> vec) {
    return vec[2];
}

void TouchCommand::execute() {
    vector<string> vec = splitS(cmd_line);
    if (!touchCommandIsValid(vec))
        return;
    string file_name = touchCommGetFileName(vec);
    struct tm time;
    string time_str = touchCommGetTime(vec);
    char *time_c = new char[time_str.length() + 1];
    string::iterator iter = time_str.begin();
    for (int i = 0; iter != time_str.end(); iter++, i++)
        time_c[i] = *iter;
    strptime(time_c, "%S:%M:%H:%d:%m:%Y", &time);
    delete[] time_c;
    time_t updated_time = mktime(&time);
    struct utimbuf time_buf;
    time_buf.actime = updated_time;
    time_buf.modtime = updated_time;
    int res = utime(file_name.c_str(), &time_buf);
    if (res == -1)
        perror("smash error: utime failed");
}



bool fareCommandIsValid(vector<string> vec) {
    if ((vec[1] == "") || (vec[2] == "") || (vec[3] == "") || !(vec[4] == "")) {
        cerr << "smash error: fare: invalid arguments" << endl;
        return false;
    }
    return true;
}


void FareCommand::execute() {
    vector<string> vec = splitS(cmd_line);
    if (!fareCommandIsValid(vec))
        return;
    string file_name = touchCommGetFileName(vec);
    int my_f0 = open(file_name.c_str(), O_RDWR, 0664);
    if (my_f0 == -1) {
        perror("smash error: open failed");
        return;
    }
    char x;
    int file_size = 0;
    int res;
    while ((res = read(my_f0, &x, 1) != 0)) {
        if (res == -1) {
            perror("smash error: read failed");
            return;
        }
        file_size++;
    }
    close(my_f0);
    int original_file_size = file_size;
    char* buff1 = new char[file_size];
    int my_f1 = open(file_name.c_str(), O_RDWR, 0664);
    if (my_f1 == -1) {
        perror("smash error: open failed");
        return;
    }
    res = read(my_f1, buff1, file_size);
    if (res == -1) {
        perror("smash error: read failed");
        return;
    }
    close(my_f1);

    string text = string(buff1);
    string src = vec[2];
    string dst = vec[3];
    int source_size = vec[2].size(), destination_size = vec[3].size();
    int change_count = 0, i = 0;
    while (i < file_size-source_size) {
        if(text.substr(i, source_size) == src) {
            string prefix = text.substr(0, i);
            string suffix = text.substr(i + source_size, file_size - (i + source_size));
            text = "";
            text += prefix;
            text += dst;
            text += suffix;
            change_count++;
            file_size = text.size();
            i += destination_size;
        }
        else {
            i++;
        }
    }

    file_size = text.size();
    const char* buff2 = text.c_str();

    int my_f2 = open(file_name.c_str(), O_RDWR | O_TRUNC, 0664);
    if (my_f2 == -1) {
        perror("smash error: open failed");
        return;
    }

    res = write(my_f2, buff2, file_size);

    if (res == -1) {
        perror("smash error: write failed");
        return;
    }
    close(my_f2);

    cout << "replaced " << change_count << " instances of the string \"" << src << "\"" << endl;
}


