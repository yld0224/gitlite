#include "../include/Repository.h"
#include "../include/Utils.h"
#include <filesystem>
#include "../include/Commit.h"
#include "../include/GitliteException.h"
#include <fstream>
#include <iostream>
void Repository::init(){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,".gitlite");
    if(Utils::isDirectory(path)){
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    if(Utils::createDirectories(path)){
        Utils::createDirectories(path+"/ref");
        Utils::createDirectories(path+"/obj");
        Commit initial_commit;
        std::string initial_id=Commit::genID(initial_commit);
        Utils::writeContents(path+"/ref"+"/master.txt",initial_id);
        Utils::writeContents(path+"/ref"+"/HEAD.txt",initial_id);
        //接下来需要把commit给序列化并且存储到obj里面，应该怎么存储?
    }
};//由于要求所有初始提交都有相同id，用时间的sha-1来代表id