#include "../include/Commit.h"
#include "../include/Utils.h"
std::string Commit::genID(Commit commit){
    if(commit.tracked_files.empty()){
        return Utils::sha1(commit.timestamp);
    }
}