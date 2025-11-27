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
        initial_commit.save();
        std::string initial_id=initial_commit.getID();
        Utils::writeContents(path+"/ref"+"/master",initial_id);
        Utils::writeContents(path+"/ref"+"/HEAD","master");
    }
};