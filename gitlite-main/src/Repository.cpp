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
        Utils::writeContents(path+"/HEAD","ref:refs/heads/master");
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
    std::string commit_id=getCommitIdFromHEAD();
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
        new_blob.save_blob(new_blob);
    }else{//没有找到同名文件，创建一个新的add文件和对应的blob
        current_stage.added_files[filename]=blob_id;
        new_blob.save_blob(new_blob);
    }
    current_stage.save_stage(current_stage);//将更改之后的stage保存
    return;
}
void Repository::commit(std::string message){
    if(message.empty()){
        Utils::exitWithMessage("Please enter a commit message.");
    }
    Commit last_commit;
    std::string last_commit_id=getCommitIdFromHEAD();
    last_commit=Commit::load(last_commit_id);
    Commit current_commit(last_commit);
    current_commit.setTime();
    current_commit.setMessage(message);
    current_commit.setParents(last_commit_id);
    //先找回上次的commit，并且复制到本次的commit上
    stage current_stage;
    current_stage=stage::load_stage();
    if(current_stage.added_files.empty()&&current_stage.removed_files.empty()){
        Utils::exitWithMessage("No changes added to the commit.");
    }
    std::map tracked_files=current_commit.getTrackedFiles();
    for(auto& [filename,blob_id]:current_stage.added_files){
        auto iter=tracked_files.find(filename);
        if(iter!=tracked_files.end()){
            iter->second=blob_id;
        }else{
            tracked_files[filename]=blob_id;
        }
    }
    for(auto&[filename,blob_id]:current_stage.removed_files){
        auto iter=tracked_files.find(filename);
        if(iter!=tracked_files.end()){
            tracked_files.erase(iter);
        }
    }
    current_commit.setTrackedFiles(current_commit,tracked_files);
    //这里是更新commit里面的跟踪文件
    current_stage.clear();
    current_stage.save_stage(current_stage);
    //这里是改变stage的状态
    std::string current_id=current_commit.genID();
    current_commit.setID(current_id);
    current_commit.save();
    //将commit改变之后保存在文件夹
    if(isDetachedHEAD()){
        std::string path=Utils::join(getGitliteDir(),"HEAD");
        Utils::writeContents(path,current_id);
    }else{
        std::string path=getPathToBranch();
        Utils::writeContents(path,current_id);
    }
    //最后应该要移动头指针和分支指针
    return;
}
void Repository::rm(std::string filename){
    std::string path=getGitliteDir();
    stage loading_stage;
    loading_stage=stage::load_stage();
    Commit loading_commit;
    std::string commit_id=getCommitIdFromHEAD();
    loading_commit=Commit::load(commit_id);
    auto current_tracked_files=loading_commit.getTrackedFiles();
    auto iter=loading_stage.added_files.find(filename);
    auto iter1=current_tracked_files.find(filename);
    //接下来根据find的情况对文件进行操作
    if(iter==loading_stage.added_files.end()&&iter1==current_tracked_files.end()){
        Utils::exitWithMessage("No reason to remove the file.");
    }
    if(iter!=loading_stage.added_files.end()){
        if(iter1==current_tracked_files.end()){
            loading_stage.added_files.erase(filename);
            loading_stage.save_stage(loading_stage);
            return;
        }else{
            std::string blob_id=iter1->second;
            loading_stage.added_files.erase(filename);
            loading_stage.removed_files[filename]=blob_id;
            loading_stage.save_stage(loading_stage);
            std::string pathToDelete=Utils::join(path,filename);
            Utils::restrictedDelete(pathToDelete);
            return;
        }//bugfix:如果在stage上而且在commit里的情况
    }else{
        if(iter1!=current_tracked_files.end()){
            std::string blob_id=iter1->second;
            loading_stage.removed_files[filename]=blob_id;
            loading_stage.save_stage(loading_stage);
            std::string pathToDelete=Utils::join(path,filename);
            Utils::restrictedDelete(pathToDelete);
            return;
        }
    }
}
void Repository::log(){
    Commit current_commit;
    std::string commit_id=getCommitIdFromHEAD();
    current_commit.load(commit_id);
    auto parents=current_commit.getParents();
    while(!parents.empty()){
        current_commit.showCommitInfo();
        current_commit.load(parents[0]);
        parents=current_commit.getParents();
    }//当前commit还存在父节点
    current_commit.showCommitInfo();//初始commit
    return;
}
void Repository::globalLog(){
    std::vector<std::string> filenames;
    std::string path=getGitliteDir();
    path=Utils::join(path,"objects");
    filenames=Utils::plainFilenamesIn(path);
    for(auto filename:filenames){
        std::stringstream ss;
        ss<<Utils::readContentsAsString(filename);
        std::string line;
        if(std::getline(ss,line)){
            if(line.substr(0,9)=="timestamp"){
                Commit current_commit;
                current_commit.load(filename);
                current_commit.showCommitInfo();
            }
        }
    }
    return;
}



//辅助功能
std::string getGitliteDir(){
    return static_cast<std::string>(std::filesystem::current_path()) + "/.gitlite";
}
std::string getPathToBranch(){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,".gitlite","HEAD");
    std::stringstream ss;
    ss<<Utils::readContentsAsString(path);
    std::string line;
    std::string pathToBranch;
    while(std::getline(ss,line)){
        auto pos=line.find(':');
        pathToBranch=line.substr(pos+1);
        auto start = pathToBranch.find_first_not_of(" ");
        if (start != std::string::npos) {pathToBranch = pathToBranch.substr(start);}
    }
    return pathToBranch=Utils::join(cwd,".gitlite",pathToBranch);
}
std::string getCommitIdFromHEAD(){
    if(!isDetachedHEAD()){
        std::string pathToBranch=getPathToBranch();
        return Utils::readContentsAsString(pathToBranch);
    }else{
        std::string path=Utils::join(getGitliteDir(),"HEAD");
        std::string content = Utils::readContentsAsString(path);
        return content;
    }
}
bool isDetachedHEAD(){
    std::string path=Utils::join(getGitliteDir(),"HEAD");
    std::string content = Utils::readContentsAsString(path);
    return content.find("ref:") == std::string::npos;
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
void blob::save_blob(blob){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,".gitlite","objects",this->blob_id);
    Utils::writeContents(path,this->blob_contents);
    return;
}