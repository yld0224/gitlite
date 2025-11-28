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
        Utils::createDirectories(path+"/refs/heads");
        Utils::createDirectories(path+"/objects");
        Commit initial_commit;
        initial_commit.save();
        std::string initial_id=initial_commit.getID();
        Utils::writeContents(path+"/refs/heads/master",initial_id);
        Utils::writeContents(path+"/HEAD","ref: refs/heads/master");
    }
}
void Repository::add(std::string filename){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,filename);
    if(!Utils::exists(path)){
        Utils::exitWithMessage("File does not exist.");
    }
    std::string content=Utils::readContentsAsString(path);
    std::string blob_id=Utils::sha1(filename,content);
    blob new_blob(blob_id,content);
    stage current_stage;
    Commit current_commit;
    std::string commit_id=getHEAD();
    current_commit=Commit::load(commit_id);//从头指针中找到commitid
    current_stage=stage::load_stage();
    std::map<std::string,std::string> commited_files=current_commit.getTrackedFiles();
    auto iter1=commited_files.find(filename);
    auto iter=current_stage.added_files.find(filename);
    auto iter2=current_stage.removed_files.find(filename);
    if(iter2!=current_stage.removed_files.end()){
        current_stage.removed_files.erase(iter2);
    }//如果在待删除区域中,去除removed标记
    if(iter1!=commited_files.end()){
        if(commited_files[filename]==new_blob.getID()){
            if(iter!=current_stage.added_files.end()){
                current_stage.added_files.erase(iter);
            }
            current_stage.save_stage(current_stage);
            return;
        }
    }//当前提交中含有与该文件版本相同的文件
    if(iter!=current_stage.added_files.end()){//在暂存区找到同名文件
        iter->second=new_blob.getID();
        new_blob.save_blob();
    }else{//没有找到同名文件，创建一个新的add文件和对应的blob
        current_stage.added_files[filename]=blob_id;
        new_blob.save_blob();
    }
    current_stage.save_stage(current_stage);//将更改之后的stage保存
    return;
}




//辅助功能
std::string getGitliteDir(){
    return static_cast<std::string>(std::filesystem::current_path()) + "/.gitlite";
}
std::string getHEAD(){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,".gitlite","HEAD");
    std::stringstream ss;
    ss<<Utils::readContentsAsString(path);
    std::string line;
    std::string pathToBranch;
    while(std::getline(ss,line)){
        auto pos=line.find(':');
        pathToBranch=line.substr(pos+1,line.length());
    }
    pathToBranch=Utils::join(cwd,".gitlite",pathToBranch);
    return Utils::readContentsAsString(pathToBranch);
}
void stage::save_stage(stage){
    std::ostringstream contents;
    for(auto&[filename,blob_id]:this->added_files){
    contents<<"added_files:"<<filename<<' '<<blob_id<<'\n';
    }
    for(auto&[filename,blob_id]:this->removed_files){
        contents<<"removed_files:"<<filename<<' '<<blob_id<<'\n';
    }
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,".gitlite","objects","stage");
    Utils::writeContents(path,contents.str());
}
stage stage::load_stage(){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,".gitlite","objects","stage");
    stage loading_stage;
    std::stringstream ss;
    if(!Utils::exists(path)){
        return loading_stage;
    }
    ss<<Utils::readContentsAsString(path);
    std::string lines;
    while(std::getline(ss,lines)){
        auto pos=lines.find(':');
        if(lines.substr(0,pos)=="added_files"){
        auto space_pos=lines.find(' ');
        std::string filename=lines.substr(pos+1,space_pos-pos-1);
        std::string blob_id=lines.substr(space_pos+1);
        loading_stage.added_files[filename]=blob_id;
      }if(lines.substr(0,pos)=="removed_files"){
        auto space_pos=lines.find(' ');
        std::string filename=lines.substr(pos+1,space_pos-pos-1);
        std::string blob_id=lines.substr(space_pos+1);
        loading_stage.removed_files[filename]=blob_id;
      }
    }
   return loading_stage;
}