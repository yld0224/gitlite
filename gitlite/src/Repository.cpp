#include "../include/Repository.h"
#include "../include/Utils.h"
#include <filesystem>
#include "../include/Commit.h"
#include "../include/GitliteException.h"
#include <fstream>
#include <iostream>
#include <algorithm>
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
    }//初始化仓库，创建基础文件夹，提交初始commit并且初始化头指针和分支
    return;
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
    std::string path = static_cast<std::string>(std::filesystem::current_path());
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
    current_commit=current_commit.load(commit_id);
    auto parents=current_commit.getParents();
    while(!parents.empty()){
        current_commit.showCommitInfo();
        current_commit=current_commit.load(parents[0]);
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
        std::string fullpath=Utils::join(path,filename);
        ss<<Utils::readContentsAsString(fullpath);
        std::string line;
        if(std::getline(ss,line)){
            if(line.substr(0,9)=="timestamp"){
                Commit current_commit;
                current_commit=current_commit.load(filename);
                current_commit.showCommitInfo();
            }
        }
    }//将储存在objects中的文件扫一遍，包括commit和blob和stage
    return;
}
void Repository::find(std::string commit_message){
    size_t pos1=commit_message.find_first_of("\"");
    size_t pos2=commit_message.find_last_of("\"");
    std::string message;
    size_t len=commit_message.length();
    if(pos1==0&&pos2==len-1){
        message=commit_message.substr(1,len-2);
    }else{
        message=commit_message;
    }
    //先去除message上可能的引号
    std::vector<std::string> filenames;
    std::string path=getGitliteDir();
    path=Utils::join(path,"objects");
    filenames=Utils::plainFilenamesIn(path);
    bool has_found=false;
    for(auto filename:filenames){
        std::stringstream ss;
        std::string fullpath=Utils::join(path,filename);
        ss<<Utils::readContentsAsString(fullpath);
        std::string line;
        std::string essay;
        if(std::getline(ss,line)){
            if(line.length()<9||line.substr(0,9)!="timestamp"){continue;}
        }//不是commit文件
        while(std::getline(ss,line)){
           if(line.length()<7){continue;}
           if(line.substr(0,7)=="message"){
                essay=line.substr(8);
                break;
            }
        }
        if(essay.find(message)!=std::string::npos){
            has_found=true;
            Commit loading_commit;
            loading_commit=loading_commit.load(filename);
            std::cout<<loading_commit.getID()<<std::endl;
        }
    }
    if(!has_found){
        Utils::exitWithMessage("Found no commit with that message.");
    }
    return;
}
void Repository::checkoutFile(std::string filename){
    std::string commit_id=getCommitIdFromHEAD();
    checkoutFileInCommit(commit_id,filename);
}
void Repository::checkoutFileInCommit(std::string commit_id,std::string filename){
    std::string path=getGitliteDir();
    path=Utils::join(path,"objects");
    auto filenames=Utils::plainFilenamesIn(path);
    Commit loading_commit;
    bool has_found_commit=false;
    for(auto _commit_id:filenames){
        if(_commit_id.substr(0,commit_id.length())==commit_id){//防止压缩的id长度不相等报错
            has_found_commit=true;
            loading_commit=loading_commit.load(_commit_id);
            break;
        }
    }
    if(!has_found_commit){
        Utils::exitWithMessage("No commit with that id exists.");
    }
    //先找到对应id的commit
    blob loading_blob;
    auto current_files=loading_commit.getTrackedFiles();
    bool has_found_file=false;
    for(auto&[_filename,_blob_id]:current_files){
        if(filename==_filename){
            loading_blob.load_blob(_blob_id);
            has_found_file=true;
            break;
        }
    }
    if(!has_found_file){
        Utils::exitWithMessage("File does not exist in that commit.");
    }
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    cwd=Utils::join(cwd,filename);
    Utils::writeContents(cwd,loading_blob.getContent());
    return;
}
void Repository::status(){
    std::cout<<"=== Branches ==="<<std::endl;
    std::string current_branch_name;
    if(!isDetachedHEAD()){
        std::string path=getPathToBranch();
        size_t pos=path.find_last_of("/");
        current_branch_name=path.substr(pos+1);
    }
    if(!current_branch_name.empty()){
        std::cout<<"*"<<current_branch_name<<std::endl;
    }
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","heads");
    auto filenames=Utils::plainFilenamesIn(path);
    std::sort(filenames.begin(),filenames.end());//字典序
    for(auto filename:filenames){
        if(filename!=current_branch_name){
            std::cout<<filename<<std::endl;
        }
    }
    std::cout<<std::endl;
    //处理分支列举
    stage loading_stage;
    loading_stage=loading_stage.load_stage();
    std::cout<<"=== Staged Files ==="<<std::endl;
    for(auto file:loading_stage.added_files){
        std::cout<<file.first<<std::endl;
    }
    std::cout<<std::endl;
    std::cout<<"=== Removed Files ==="<<std::endl;
    for(auto file:loading_stage.removed_files){
        std::cout<<file.first<<std::endl;
    }
    std::cout<<std::endl;
    //处理stage的文件
    std::cout<<"=== Modifications Not Staged For Commit ==="<<std::endl;
    std::cout<<std::endl;
    std::cout<<"=== Untracked Files ==="<<std::endl;
    //暂时还没到subtask6
}
void Repository::checkoutBranch(std::string branchname){
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","heads");
    auto branchnames=Utils::plainFilenamesIn(path);
    bool has_found=false;
    for(auto name:branchnames){
        if(name==branchname){
            has_found=true;
            break;
        }
    }
    if(!has_found){
        Utils::exitWithMessage("No such branch exists.");
    }
    path=Utils::join(path,branchname);
    if(!isDetachedHEAD()){
        std::string path=getPathToBranch();
        size_t pos=path.find_last_of("/");
        if(path.substr(pos+1)==branchname){
            Utils::exitWithMessage("No need to checkout the current branch.");
        }
    }
    //处理非法的branchname
    Commit last_commit;
    Commit new_commit;
    last_commit.load(getCommitIdFromHEAD());
    new_commit.load(Utils::readContentsAsString(path));
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    auto new_files=new_commit.getTrackedFiles();
    auto last_files=last_commit.getTrackedFiles();
    for(auto file:new_files){
        auto&[filename,blob_id]=file;
        auto iter=last_files.find(filename);
        if(iter==last_files.end()){
            std::string path=Utils::join(cwd,filename);
            if(Utils::isFile(path))
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }//在checkout的commit中，没有被当前commit跟踪，而且在工作目录中
    for(auto file:last_files){
        auto&[filename,blob_id]=file;
        auto iter=new_files.find(filename);
        if(iter==new_files.end()){
            std::string path=Utils::join(cwd,filename);
            if(Utils::isFile(path)){
                Utils::restrictedDelete(path);
            }
        }
    }//被跟踪但不在checkout的commit的文件中，删除
    for(auto file:new_files){
        auto&[filename,blob_id]=file;
        blob loading_blob;
        loading_blob.load_blob(blob_id);
        std::string path=Utils::join(cwd,filename);
        if(Utils::isFile(path)){
            Utils::writeContents(path,loading_blob.getContent());
        }
    }//用checkout分支的文件覆盖工作目录中的文件
    std::string pathToHead=Utils::join(getGitliteDir(),"HEAD");
    std::string message="ref:refs/heads/"+branchname;
    Utils::writeContents(pathToHead,message);
    stage new_stage;
    new_stage=new_stage.load_stage();
    new_stage.clear();
    new_stage.save_stage(new_stage);
    //最后移动头指针的位置,并且清空当前stage
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
void blob::load_blob(std::string blob_id){
    std::string path=getGitliteDir();
    path=Utils::join(path,"objects",blob_id);
    std::string content=Utils::readContentsAsString(path);
    this->blob_contents=content;
    return;
}
