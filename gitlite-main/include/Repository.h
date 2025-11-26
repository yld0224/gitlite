#ifndef REPOSITORY_H
#define REPOSITORY_H
#include<vector>
#include<map>
#include"Commit.h"
class blob{
    private:
    std::string blob_id;
    std::vector<unsigned char> blob_contents;
};
class stage{
    std::map<std::string,std::string> added_files;
    std::map<std::string,std::string> removed_files;
    void save_stage();
    void load_stage();
};
//repository类不用于存储数据，而是在需要的时候从磁盘读取
//repository类包含所有的函数方法
class Repository{
    private:

    public:
    Repository()=default;
    ~Repository()=default;
    void init();
    void addRemote(std::string,std::string);
    void rmRemote(std::string);
    void add(std::string);
    void commit(std::string);
    void rm(std::string);
    void log();
    void globalLog();
    void find(std::string);
    void status();
    void checkoutBranch(std::string);
    void checkoutFile(std::string);
    void checkoutFileInCommit(std::string,std::string);
    void branch(std::string);
    void rmBranch(std::string);
    void reset(std::string);
    void merge(std::string);
    void push(std::string,std::string);
    void fetch(std::string,std::string);
    void pull(std::string,std::string);
};
std::string getGitliteDir();//返回gitlite的文件路径
#endif // REPOSITORY_H
