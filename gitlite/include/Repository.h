#ifndef REPOSITORY_H
#define REPOSITORY_H
#include<vector>
#include<map>
#include"Commit.h"
class blob{
    private:
    std::string blob_id;
    std::string blob_contents;
    public:
    blob(std::string _id,std::string _content):blob_id(_id),blob_contents(_content){}
    blob()=default;
    std::string getContent(){return blob_contents;}
    std::string getID(){return blob_id;}
    void save_blob(blob);
    void load_blob(std::string blob_id);
};
class stage{
    public:
    std::map<std::string,std::string> added_files;//分别代表文件名和blob_id
    std::map<std::string,std::string> removed_files;
    public:
    void save_stage(stage);
    static stage load_stage();
    void clear(){
        this->added_files.clear();
        this->removed_files.clear();
        return;
    }
};
//repository类不用于存储数据，而是在需要的时候从磁盘读取
//repository类包含所有的函数方法
class Repository{
    private:

    public:
    Repository()=default;
    ~Repository()=default;
    void init();//初始化仓库，创建文件夹
    void add(std::string filename);//将文件放上stage
    void commit(std::string message);
    void rm(std::string filename);
    void log();
    void globalLog();
    void find(std::string commit_message);
    void checkoutFile(std::string filename);
    void checkoutFileInCommit(std::string commit_id,std::string filename);
    void status();
    void checkoutBranch(std::string branchname);
};
std::string getGitliteDir();//返回gitlite的文件路径
std::string getPathToBranch();//返回到当前branch的路径
bool isDetachedHEAD();
std::string getCommitIdFromHEAD();//返回HEAD指针里的文件内容
#endif // REPOSITORY_H
