#include "../include/Repository.h"
#include "../include/Utils.h"
#include <filesystem>
#include "../include/Commit.h"
#include "../include/GitliteException.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <queue>
void Repository::init(){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=Utils::join(cwd,".gitlite");
    if(Utils::isDirectory(path)){
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    if(Utils::createDirectories(path)){
        Utils::createDirectories(path+"/refs/heads");
        Utils::createDirectories(path+"/refs/remotes");
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
    Commit current_commit;
    std::string commit_id=getCommitIdFromHEAD();
    current_commit=current_commit.load(commit_id);
    auto files=current_commit.getTrackedFiles();
    auto staged_files=loading_stage.added_files;
    auto removed_files=loading_stage.removed_files;
    for(auto file:files){
        auto& [filename,blob_id]=file;
        std::string path=static_cast<std::string>(std::filesystem::current_path());
        path=Utils::join(path,filename);
        auto iter=staged_files.find(filename);
        blob loading_blob;
        loading_blob.load_blob(blob_id);
        if(Utils::isFile(path)){
            std::string current_content=Utils::readContentsAsString(path);
            std::string original_content=loading_blob.getContent();
            if(current_content!=original_content&&iter==staged_files.end()){
                std::cout<<filename+" (modified)"<<std::endl;
            }
        }//内容不同且没有在stage上
        else{
            if(removed_files.find(filename)==removed_files.end()){
                std::cout<<filename+" (deleted)"<<std::endl;
            }//文件已经被删除但removedfiles没有记录
        }
    }
    for(auto file:staged_files){
        auto& [filename,blob_id]=file;
        std::string cwd=static_cast<std::string>(std::filesystem::current_path());
        std::string path=Utils::join(cwd,filename);
        if(!Utils::isFile(path)){
            std::cout<<filename+" (deleted)"<<std::endl;//已经被删除，却在stage上
        }else{
            std::string current_content=Utils::readContentsAsString(path);
            blob loading_blob;
            loading_blob.load_blob(blob_id);
            std::string staged_content=loading_blob.getContent();
            if(current_content!=staged_content){
                std::cout<<filename+" (modified)"<<std::endl;
            }//stage的文件内容和工作目录下不同
        }
    }
    std::cout<<std::endl;
    std::cout<<"=== Untracked Files ==="<<std::endl;
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    for(auto file:removed_files){
        std::string full_path=Utils::join(cwd,file.first);
        if(Utils::isFile(full_path)){
            std::cout<<file.first<<std::endl;
        }//本已被删除却存在
    }
    std::vector<std::string> working_files=Utils::plainFilenamesIn(cwd);
    for(auto filename:working_files){
        auto iter1=files.find(filename);
        auto iter2=staged_files.find(filename);
        auto iter3=removed_files.find(filename);
        if(iter1==files.end()&&iter2==staged_files.end()&&iter3==removed_files.end()){
            std::cout<<filename<<std::endl;
        }//在工作区却不在commit中也不在stage上
    }
}
void Repository::checkoutBranch(std::string branchname){
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","heads");
    std::string branch_path=Utils::join(path,branchname);
    if(!Utils::isFile(branch_path)){
        Utils::exitWithMessage("No such branch exists.");
    }
    path=branch_path;
    if(!isDetachedHEAD()){
        std::string path=getPathToBranch();
        std::string current_branch;
        size_t heads_pos=path.find("refs/heads/");
        if(heads_pos!=std::string::npos){
            current_branch=path.substr(heads_pos+11);
        }
        if(current_branch==branchname){
            Utils::exitWithMessage("No need to checkout the current branch.");
        }
    }
    //处理非法的branchname
    Commit last_commit;
    Commit new_commit;
    last_commit=last_commit.load(getCommitIdFromHEAD());
    new_commit=new_commit.load(Utils::readContentsAsString(path));//bugfix:又不小心丢弃了返回值
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
                std::remove(path.c_str());
            }
        }
    }//被跟踪但不在checkout的commit的文件中，删除
    for(auto file:new_files){
        auto&[filename,blob_id]=file;
        blob loading_blob;
        loading_blob.load_blob(blob_id);
        std::string path=Utils::join(cwd,filename);
        Utils::writeContents(path,loading_blob.getContent());//bugfix:不应该检查文件是否存在
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
void Repository::branch(std::string branchname){
    std::string commit_id=getCommitIdFromHEAD();//得到目前commitid
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","heads",branchname);
    if(Utils::isFile(path)){
        Utils::exitWithMessage("A branch with that name already exists.");
    }
    Utils::writeContents(path,commit_id);//在heads里面创建新的分支
    return;
}
void Repository::rmBranch(std::string branchname){
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","heads");
    auto filenames=Utils::plainFilenamesIn(path);
    bool has_found=false;
    for(auto filename:filenames){
        if(filename==branchname){
            has_found=true;
            break;
        }
    }
    if(!has_found){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    if(!isDetachedHEAD()){
        std::string current_path=getPathToBranch();
        size_t pos=current_path.find_last_of("/");
        std::string current_branchname=current_path.substr(pos+1);
        if(branchname==current_branchname){
            Utils::exitWithMessage("Cannot remove the current branch.");
        }
    }//处理异常情况
    path=Utils::join(path,branchname);
    std::remove(path.c_str());
    //简单将分支的指针删除
    return;
}
void Repository::reset(std::string commit_id){
    std::string path=getGitliteDir();
    path=Utils::join(path,"objects");
    bool has_found=false;
    auto filenames=Utils::plainFilenamesIn(path);
    for(auto filename:filenames){
        if(filename==commit_id){
            has_found=true;
            break;
        }
    }
    if(!has_found){
        Utils::exitWithMessage("No commit with that id exists.");
    }//没有找到当前commit_id
    Commit last_commit;
    Commit new_commit;
    last_commit=last_commit.load(getCommitIdFromHEAD());
    new_commit=new_commit.load(commit_id);
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
                std::remove(path.c_str());
            }
        }
    }//被跟踪但不在checkout的commit的文件中，删除
    for(auto file:new_files){
        auto&[filename,blob_id]=file;
        blob loading_blob;
        loading_blob.load_blob(blob_id);
        std::string path=Utils::join(cwd,filename);
        Utils::writeContents(path,loading_blob.getContent());//bugfix:不应检查文件是否存在
    }//用checkout分支的文件覆盖工作目录中的文件
    std::string pathToHead=Utils::join(getGitliteDir(),"HEAD");
    std::string message=commit_id;
    if(isDetachedHEAD()){
        Utils::writeContents(pathToHead,message);//如果是detached，则改变head指针
    }else{
        std::string pathToBranch=getPathToBranch();
        Utils::writeContents(pathToBranch,message);
    }
    stage new_stage;
    new_stage=new_stage.load_stage();
    new_stage.clear();
    new_stage.save_stage(new_stage);
    //最后移动头指针的位置,并且清空当前stage
    return;
}
void Repository::merge(std::string branchname){
    std::string current_branchname;
    stage current_stage;
    current_stage=current_stage.load_stage();
    if(!(current_stage.added_files.empty())||!(current_stage.removed_files.empty())){
        Utils::exitWithMessage("You have uncommitted changes.");
    }
    std::string heads_path=getGitliteDir();
    heads_path=Utils::join(heads_path,"refs","heads");
    std::string branch_path=Utils::join(heads_path,branchname);
    if(!Utils::isFile(branch_path)){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    if(!isDetachedHEAD()){
        std::string path=getPathToBranch();
        size_t pos=path.find_last_of("/");
        current_branchname=path.substr(pos+1);
        if(branchname==current_branchname){
            Utils::exitWithMessage("Cannot merge a branch with itself.");
        }
    }//处理merge之前异常情况
    std::string commit_id=getCommitIdFromHEAD();
    Commit current_commit;
    current_commit=current_commit.load(commit_id);
    heads_path=Utils::join(heads_path,branchname);
    std::string other_commit_id;
    other_commit_id=Utils::readContentsAsString(heads_path);
    Commit other_commit;
    other_commit=other_commit.load(other_commit_id);
    std::string LCA;
    LCA=getLCA(current_commit,other_commit);
    //找到LCA
    if(LCA==other_commit_id){
        Utils::exitWithMessage("Given branch is an ancestor of the current branch.");
    }else if(LCA==commit_id){//bugfix:LCA=commit_id
        Repository* repo=new Repository;
        repo->checkoutBranch(branchname);
        delete repo;
        Utils::exitWithMessage("Current branch fast-forwarded.");
    }//两种各自是对方祖先的情况
    Commit parent_commit;
    parent_commit=parent_commit.load(LCA);
    auto old_files=parent_commit.getTrackedFiles();
    auto my_files=current_commit.getTrackedFiles();
    auto other_files=other_commit.getTrackedFiles();
    std::string path_to_file=static_cast<std::string>(std::filesystem::current_path());
    bool has_conflicts=false;
    for(auto old_file:old_files){
        auto my_iter=my_files.find(old_file.first);
        auto other_iter=other_files.find(old_file.first);
        if(my_iter==my_files.end()&&other_iter!=other_files.end()&&other_iter->second!=old_file.second){
            std::string cwd=static_cast<std::string>(std::filesystem::current_path());
            std::string full_filepath = Utils::join(cwd, old_file.first);
            if (Utils::isFile(full_filepath)) {
                Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
            }
        }
    }
    for(auto other_file:other_files){
        auto my_iter=my_files.find(other_file.first);
        auto old_iter=old_files.find(other_file.first);
        if(my_iter==my_files.end()&&old_iter==old_files.end()){
            std::string full_filepath=Utils::join(path_to_file,other_file.first);
            if(Utils::isFile(full_filepath)){
                Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
            }
        }   
    }//对仓库中的文件的保护，防止直接覆盖
    //(important)bugfix:对异常状态的判定提前，防止stage存在状态污染
    for(auto old_file:old_files){
       auto my_iter=my_files.find(old_file.first);
       auto other_iter=other_files.find(old_file.first);
       if(my_iter!=my_files.end()&&my_iter->second==old_file.second){
            if(other_iter!=other_files.end()&&other_iter->second!=old_file.second){
                blob loading_blob;
                loading_blob.load_blob(other_iter->second);
                std::string full_filepath=Utils::join(path_to_file,old_file.first);
                Utils::writeContents(full_filepath,loading_blob.getContent());
                stage temp_stage;
                temp_stage=temp_stage.load_stage();
                temp_stage.added_files[other_iter->first]=other_iter->second;
                temp_stage.save_stage(temp_stage);
            }//other分支修改了而当前分支没修改,听other的
        }
        if(my_iter!=my_files.end()&&my_iter->second!=old_file.second){
            if(other_iter!=other_files.end()&&other_iter->second==old_file.second){
                continue;
            }
        }//当前分支修改了而other没修改，听当前分支的
        if(my_iter!=my_files.end()&&other_iter!=other_files.end()){
            if(my_iter->second==other_iter->second){
                continue;//当前分支和other分支做出相同修改，不改变
            }else if(my_iter->second!=old_file.second&&other_iter->second!=old_file.second){//bugfix:冲突判定更严
                has_conflicts=true;
                std::cout<<"Encountered a merge conflict."<<std::endl;
                stage temp_stage;
                temp_stage=temp_stage.load_stage();
                temp_stage.added_files[old_file.first]=markConflicts(old_file.first,my_iter->second,other_iter->second);
                temp_stage.save_stage(temp_stage);
            }//当前分支和other分支做出不同修改，冲突
        }
        if(my_iter==my_files.end()&&other_iter==other_files.end()){
            continue;
        }//被两个分支一起删除的文件
        if(my_iter!=my_files.end()&&my_iter->second==old_file.second){
            if(other_iter==other_files.end()){
                std::string full_filepath=Utils::join(path_to_file,old_file.first);
                Utils::restrictedDelete(full_filepath);
                stage loading_stage;
                loading_stage=loading_stage.load_stage();
                loading_stage.removed_files[my_iter->first]=my_iter->second;
                loading_stage.save_stage(loading_stage);
            }
        }//当前分支没有修改但被other分支删除的，听other的
        if(my_iter==my_files.end()&&other_iter!=other_files.end()&&other_iter->second!=old_file.second){
            std::cout<<"Encountered a merge conflict."<<std::endl;
            has_conflicts=true;
            stage temp_stage;
            temp_stage=temp_stage.load_stage();
            temp_stage.added_files[old_file.first]=markConflicts(old_file.first,"deleted!",other_iter->second);
            temp_stage.save_stage(temp_stage);
        }//当前分支删除，other没有删除并且做出修改，冲突
        if(my_iter!=my_files.end()&&my_iter->second!=old_file.second&&other_iter==other_files.end()){
            std::cout<<"Encountered a merge conflict."<<std::endl;
            has_conflicts=true;
            stage temp_stage;
            temp_stage=temp_stage.load_stage();
            temp_stage.added_files[old_file.first]=markConflicts(old_file.first,my_iter->second,"deleted!");
            temp_stage.save_stage(temp_stage);
        }//当前分支没有删除并且做出修改，other删除，冲突
    }
    for(auto other_file:other_files){
        auto my_iter=my_files.find(other_file.first);
        auto old_iter=old_files.find(other_file.first);
        if(my_iter==my_files.end()&&old_iter==old_files.end()){
            Repository* repo=new Repository;
            repo->checkoutFileInCommit(other_commit.getID(), other_file.first);//bugfix:checkout函数选错了
            delete repo;
            stage temp_stage;
            temp_stage=temp_stage.load_stage();
            temp_stage.added_files[other_file.first]=other_file.second;
            temp_stage.save_stage(temp_stage);
        }//仅存在于other分支的文件
        if(my_iter!=my_files.end()&&old_iter==old_files.end()){
            if(other_file.second!=my_iter->second){
                std::cout<<"Encountered a merge conflict."<<std::endl;
                has_conflicts=true;
                stage temp_stage;
                temp_stage=temp_stage.load_stage();
                temp_stage.added_files[other_file.first]=markConflicts(other_file.first,my_iter->second,other_file.second);
                temp_stage.save_stage(temp_stage);
            }
        }//不在LCA中存在，但当前分支和other分支都存在且文件不同,冲突
    }
    std::string path=getPathToBranch();//如果是detachedHEAD可能不需要考虑
    size_t pos=path.find_last_of("/");
    current_branchname=path.substr(pos+1);
    mergeCommit(current_branchname,branchname,commit_id,other_commit_id);
    //合并完成自动提交
    return;
}
void Repository::addRemote(std::string remotename,std::string remotepath){
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","remotes",remotename);
    if(Utils::isFile(path)){
        Utils::exitWithMessage("A remote with that name already exists.");
    }
    Utils::writeContents(path,remotepath);//将remote记录在ref的另一文件夹内
    return;
}
void Repository::rmRemote(std::string remotename){
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","remotes",remotename);
    if(!Utils::isFile(path)){
        Utils::exitWithMessage("A remote with that name does not exist.");
    }
    std::remove(path.c_str());
    return;
}
void Repository::push(std::string remotename,std::string branchname){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","remotes",remotename);
    std::string remote_path=Utils::readContentsAsString(path);
    remote_path=std::filesystem::absolute(remote_path).string();
    if(!Utils::isDirectory(remote_path)){
        Utils::exitWithMessage("Remote directory not found.");
    }
    std::string path_to_remote_head=Utils::join(remote_path,"HEAD");
    std::string content=Utils::readContentsAsString(path_to_remote_head);
    size_t pos=content.find(":");
    std::string commit_id;
    if(pos==std::string::npos){
        commit_id=content;
    }else{
        std::string path_to_remote_branch=content.substr(pos+1);
        path_to_remote_branch=Utils::join(remote_path,path_to_remote_branch);
        commit_id=Utils::readContentsAsString(path_to_remote_branch);
    }
    std::string path_to_current_objects=Utils::join(getGitliteDir(),"objects");
    auto files=Utils::plainFilenamesIn(path_to_current_objects);
    bool has_found=false;
    for(auto filename:files){
        if(filename==commit_id){
            has_found=true;
            break;
        }
    }
    if(!has_found){Utils::exitWithMessage("Please pull down remote changes before pushing.");}
    //先处理可能的异常情况
    Commit loading_commit;
    loading_commit=loading_commit.load(getCommitIdFromHEAD());
    std::string local_commit_id=loading_commit.getID();
    std::string rwd=remote_path.substr(0,remote_path.length()-9);
    remoteCommit(loading_commit,rwd,cwd);
    //把文件复制到remote里面
    std::string path_to_remote_branch=Utils::join(remote_path,"refs","heads",branchname);
    Utils::writeContents(path_to_remote_branch, local_commit_id);
    //改变指针位置
    return;
}
void Repository::fetch(std::string remotename,std::string branchname){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path=getGitliteDir();
    path=Utils::join(path,"refs","remotes",remotename);
    std::string remote_path=Utils::readContentsAsString(path);
    std::string path_to_remote_head=Utils::join(remote_path,"HEAD");
    if(!Utils::isDirectory(remote_path)){
        Utils::exitWithMessage("Remote directory not found.");
    }
    std::string path_to_remote_objects=Utils::join(remote_path,"objects");
    std::string path_to_remote_branch=Utils::join(remote_path,"refs","heads");
    bool has_found=false;
    auto filenames=Utils::plainFilenamesIn(path_to_remote_branch);
    for(auto filename:filenames){
        if(filename==branchname){
            has_found=true;
        }
    }
    if(!has_found){
        Utils::exitWithMessage("That remote does not have that branch.");
    }
    //先处理可能的异常情况
    std::string path_to_current_objects=Utils::join(getGitliteDir(),"objects");
    auto files=Utils::plainFilenamesIn(path_to_remote_objects);
    std::string path_to_remote=remote_path.substr(0,remote_path.length()-9);
    for(auto filename:files){
        std::string full_path=Utils::join(path_to_remote_objects,filename);
        std::string content=Utils::readContentsAsString(full_path);
        if(content.length()<9||content.substr(0,9)!="timestamp"){
            blob new_blob(filename,content);
            new_blob.save_blob(new_blob);
        }else{
            Commit new_commit;
            std::filesystem::current_path(path_to_remote);
            new_commit=new_commit.load(filename);
            std::filesystem::current_path(cwd);
            new_commit.save();
        }
    }//把远程的文件放到本地;
    std::string path_to_remote_branch_file = Utils::join(remote_path, "refs", "heads", branchname);
    std::string remote_commit_id = Utils::readContentsAsString(path_to_remote_branch_file);
    std::string path_to_local_branch=Utils::join(getGitliteDir(),"refs","heads");
    Utils::writeContents(Utils::join(path_to_local_branch, remotename + "/" + branchname), remote_commit_id);
    std::filesystem::current_path(cwd);
    return;
}
void Repository::pull(std::string remotename,std::string branchname){
    this->fetch(remotename,branchname);
    std::string full_branchname=Utils::join(remotename,branchname);
    this->merge(full_branchname);
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
void mergeCommit(std::string branchname1,std::string branchname2,std::string commit_id1,std::string commit_id2){
    Commit last_commit;
    std::string last_commit_id=getCommitIdFromHEAD();
    last_commit=Commit::load(last_commit_id);
    Commit current_commit(last_commit);
    current_commit.setTime();
    current_commit.setMessage("Merged "+branchname2+" into "+branchname1+".");//bugfix:shitty '.'
    current_commit.setParents(commit_id1,commit_id2);
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
    current_stage.clear();
    current_stage.save_stage(current_stage);
    std::string current_id=current_commit.genID();
    current_commit.setID(current_id);
    current_commit.save();
    if(isDetachedHEAD()){
        std::string path=Utils::join(getGitliteDir(),"HEAD");
        Utils::writeContents(path,current_id);
    }else{
        std::string path=getPathToBranch();
        Utils::writeContents(path,current_id);
    }
    return;
}
std::string markConflicts(std::string filename,std::string blob_id1,std::string blob_id2){
    std::string cwd=static_cast<std::string>(std::filesystem::current_path());
    std::string path_to_file=Utils::join(cwd,filename);
    std::string path_to_blob=getGitliteDir();
    path_to_blob=Utils::join(path_to_blob,"objects");
    std::stringstream ss;
    if(blob_id1=="deleted!"){
        path_to_blob=Utils::join(path_to_blob,blob_id2);
        std::string content=Utils::readContentsAsString(path_to_blob);
        ss<<"<<<<<<< HEAD\n";
        ss<<"=======\n";
        ss<<content;
        if (content.back() != '\n') ss << "\n";
        ss<<">>>>>>>\n";
        Utils::writeContents(path_to_file,ss.str());//bugfix:endl?
    }
    else if(blob_id2=="deleted!"){
        path_to_blob=Utils::join(path_to_blob,blob_id1);
        std::string content=Utils::readContentsAsString(path_to_blob);
        ss<<"<<<<<<< HEAD\n";
        ss<<content;
        if (content.back() != '\n') ss << "\n";
        ss<<"=======\n";
        ss<<">>>>>>>\n";
        Utils::writeContents(path_to_file,ss.str());
    }else{
        std::string path_to_blob1=Utils::join(path_to_blob,blob_id1);
        std::string path_to_blob2=Utils::join(path_to_blob,blob_id2);
        std::string content1=Utils::readContentsAsString(path_to_blob1);
        std::string content2=Utils::readContentsAsString(path_to_blob2);
        ss<<"<<<<<<< HEAD\n";
        ss<<content1;
        if (content1.back() != '\n') ss << "\n";
        ss<<"=======\n";
        ss<<content2;
        if (content2.back() != '\n') ss << "\n";
        ss<<">>>>>>>\n";
        Utils::writeContents(path_to_file,ss.str());//bugfix:忘记写入了
    }//在工作区直接修改文件
    std::string new_blob_id=Utils::sha1(ss.str());
    blob new_blob(new_blob_id,ss.str());
    new_blob.save_blob(new_blob);//创建文件对应的新blob
    return new_blob_id;
}
std::string getLCA(Commit current_commit, Commit other_commit) {
    std::unordered_set<std::string> visited;
    std::queue<std::string> q;
    q.push(current_commit.getID());
    visited.insert(current_commit.getID());
    while (!q.empty()) {
        std::string curr_id = q.front();
        q.pop();
        Commit node = Commit::load(curr_id); 
        std::vector<std::string> parents = node.getParents();
        for (const auto& parent_id : parents) {
            if (visited.find(parent_id) == visited.end()) {
                visited.insert(parent_id);
                q.push(parent_id);
            }
        }
    }
    std::queue<std::string> q2;
    std::unordered_set<std::string> visited_other;
    q2.push(other_commit.getID());
    visited_other.insert(other_commit.getID());
    while (!q2.empty()) {
        std::string curr_id = q2.front();
        q2.pop();
        if (visited.find(curr_id) != visited.end()) {
            return curr_id;
        }
        Commit node = Commit::load(curr_id);
        std::vector<std::string> parents = node.getParents();
        for (const auto& parent_id : parents) {
            if (visited_other.find(parent_id) == visited_other.end()) {
                visited_other.insert(parent_id);
                q2.push(parent_id);
            }
        }
    }
    return "LCA not found";//不可能执行到这里 
}
void remoteCommit(Commit loaded_commit,std::string pathToRemote,std::string pathToCurrent){
    Commit current_commit(loaded_commit);
    std::string commit_id=current_commit.getID();
    std::string remote_objects_path=Utils::join(pathToRemote,".gitlite","objects");
    std::string local_objects_path=Utils::join(pathToCurrent,".gitlite","objects");
    while(1){
        std::string remote_commit_path=Utils::join(remote_objects_path,current_commit.getID());
        if(Utils::isFile(remote_commit_path)){
            break;
        }
        auto tracked_files=current_commit.getTrackedFiles();
        for(auto file:tracked_files){
            auto&[filename,blob_id]=file;
            std::string local_blob_path=Utils::join(local_objects_path,blob_id);
            std::string remote_blob_path=Utils::join(remote_objects_path,blob_id);
            if(!Utils::isFile(remote_blob_path)){
                std::string content=Utils::readContentsAsString(local_blob_path);
                Utils::writeContents(remote_blob_path,content);
            }
        }
        std::string local_commit_path=Utils::join(local_objects_path,current_commit. getID());
        std::string commit_content=Utils::readContentsAsString(local_commit_path);
        Utils::writeContents(remote_commit_path,commit_content);
        if(current_commit.getParents().empty()){break;}
        current_commit=current_commit.load(current_commit.getParents()[0]);
    }
    return;
}//bugfix:原版采用切换路径，逻辑脆弱。现直接采用绝对路径。