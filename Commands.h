#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <stack>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


std::vector<std::string> splitS(std::string cmd_line);

void stopLastJob();

void updateLastPid(int pid);
bool jobIsStopped(int pid);
int getLastPid();
bool checkIfContainAndStop(int pid);

class Command {
public:
    const char *cmd_line;

    Command(const char *cmd_line) : cmd_line(cmd_line) {}

    virtual ~Command() = default;

    virtual void execute() = 0;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

    virtual ~ExternalCommand() = default;

    void execute() override;
};

class PipeCommand : public Command {
    int index;
public:
    PipeCommand(const char *cmd_line, int index) : Command(cmd_line), index(index) {}

    virtual ~PipeCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    int index;
public:
    explicit RedirectionCommand(const char *cmd_line, int index) : Command(cmd_line), index(index) {}

    virtual ~RedirectionCommand() {}

    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
    char **plastPwd;
public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;


class QuitCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
    public:
        std::string cmd_line;
        time_t time0;
        int pid;
        int jobID;
        bool IS_STOPPED;
    public:
        JobEntry(const std::string cmd_line, time_t start_time, int pid, int jodid, bool stopped) : cmd_line(cmd_line),
                                                                                                    time0(start_time),
                                                                                                    pid(pid),
                                                                                                    jobID(jodid),
                                                                                                    IS_STOPPED(
                                                                                                            stopped) {}

    };

    std::vector<JobEntry> stopped_jobs;
    std::vector<JobEntry> jobs;
    std::vector<JobEntry> all_jobs;
    std::string last_dir;
    std::string current_dir;
public:
    JobsList() : stopped_jobs(), jobs(), all_jobs(), last_dir(""), current_dir("") {
//        for (int i = 0; i < 100; ++i) {
//            used_id[i] = false;
//        }
    }

    ~JobsList() = delete;

    void updateJobTime(int jobID);

    void addJob(std::string cmd_l, int pid, bool isStopped = false);

    void printJobsList();

    void printJobsListQuit();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    JobEntry *getJobByPid(int pid);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    void sendSignalToJobs(int SIG);

    void convertToUnstopped(int jobID);

    void convertToStopped(int jobID);

    void listUndo();
};

class JobsCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~ForegroundCommand() = default;

    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList *jobs;
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~BackgroundCommand() = default;

    void execute() override;
};

class TailCommand : public BuiltInCommand {
public:
    TailCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

    virtual ~TailCommand() {}

    void execute() override;
};

class TouchCommand : public BuiltInCommand {
public:
    TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

    virtual ~TouchCommand() {}

    void execute() override;
};

class FareCommand : public BuiltInCommand {
public:
    FareCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

    virtual ~FareCommand() {}

    void execute() override;
};


class SmallShell {
private:
    SmallShell();

public:
    JobsList *jobs_list;
	int pid;

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    // ~SmallShell();

    void executeCommand(const char *cmd_line);
};

#endif //SMASH_COMMAND_H_



