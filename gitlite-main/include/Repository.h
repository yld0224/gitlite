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
class stage{};
class Repository{
    private:
    std::string HEAD;
    std::vector<blob> current_blobs;
    stage current_stage;
    std::vector<Commit> current_commits;
    public:
    Repository(){};//构造函数，需要读取当前的head指针并对commit进行初始化
    ~Repository()=default;//析构函数，在程序结束自动析构
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
