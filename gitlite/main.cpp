#include <iostream>
#include <vector>
#include <string>
#include "include/Repository.h"
#include "include/Utils.h"

void checkCWD() {
    if (!Utils::isDirectory(getGitliteDir())) {
        Utils::exitWithMessage("Not in an initialized Gitlite directory.");
    }
}

void checkNoArgs(const std::vector<std::string>& args) {
    if (args.empty()) {
        Utils::exitWithMessage("Please enter a command.");
    }
}

void checkArgsNum(const std::vector<std::string>& args, int n) {
    if (static_cast<int>(args.size()) != n) {
        Utils::exitWithMessage("Incorrect operands.");
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(std::string(argv[i]));
    }
    
    checkNoArgs(args);
    Repository repo;
    std::string firstArg = args[0];
    
    if (firstArg == "init") {
        checkArgsNum(args, 1);
     repo.init();
    } else if (firstArg == "add-remote") {
        checkCWD();
        checkArgsNum(args, 3);
     //repo.addRemote(args[1], args[2]);
    } else if (firstArg == "rm-remote") {
        checkCWD();
        checkArgsNum(args, 2);
     //repo.rmRemote(args[1]);
    } else if (firstArg == "add") {
        checkCWD();
        checkArgsNum(args, 2);
     repo.add(args[1]);
    } else if (firstArg == "commit") {
        checkCWD();
        checkArgsNum(args, 2);
     repo.commit(args[1]);
    } else if (firstArg == "rm") {
        checkCWD();
        checkArgsNum(args, 2);
     repo.rm(args[1]);
    } else if (firstArg == "log") {
        checkCWD();
        checkArgsNum(args, 1);
     repo.log();
    } else if (firstArg == "global-log") {
        checkCWD();
        checkArgsNum(args, 1);
     repo.globalLog();
    } else if (firstArg == "find") {
        checkCWD();
        checkArgsNum(args, 2);
     repo.find(args[1]);
    } else if (firstArg == "status") {
        checkCWD();
        checkArgsNum(args, 1);
     repo.status();
    } else if (firstArg == "checkout") {
        checkCWD();
        if (args.size() == 2) {
         repo.checkoutBranch(args[1]);
        } else if (args.size() == 3) {
            if (args[1] != "--") {
                Utils::exitWithMessage("Incorrect operands.");
            }
        repo.checkoutFile(args[2]);
        } else if (args.size() == 4) {
            if (args[2] != "--") {
                Utils::exitWithMessage("Incorrect operands.");
            }
        repo.checkoutFileInCommit(args[1], args[3]);
        } else {
            Utils::exitWithMessage("Incorrect operands.");
        }
    } else if (firstArg == "branch") {
        checkCWD();
        checkArgsNum(args, 2);
    // repo.branch(args[1]);
    } else if (firstArg == "rm-branch") {
        checkCWD();
        checkArgsNum(args, 2);
    // repo.rmBranch(args[1]);
    } else if (firstArg == "reset") {
        checkCWD();
        checkArgsNum(args, 2);
    // repo.reset(args[1]);
    } else if (firstArg == "merge") {
        checkCWD();
        checkArgsNum(args, 2);
    // repo.merge(args[1]);
    } else if (firstArg == "push") {
        checkCWD();
        checkArgsNum(args, 3);
    // repo.push(args[1], args[2]);
    } else if (firstArg == "fetch") {
        checkCWD();
        checkArgsNum(args, 3);
    // repo.fetch(args[1], args[2]);
    } else if (firstArg == "pull") {
        checkCWD();
        checkArgsNum(args, 3);
    // repo.pull(args[1], args[2]);
    } else {
        std::cout << "No command with that name exists." << std::endl;
        return 0;
    }
    
    return 0;
}
