#ifndef COMMIT_H
#define COMMIT_H
#include <memory>
#include <string>
#include <ctime>
#include <map>
#include <vector>
//commit类保存的就是一次commit所包含的信息
class Commit{
    private:
    std::string timestamp;
    std::string message;
    std::string commit_id;
    std::map<std::string,std::string> tracked_files;//文件名，blod_id
    std::vector<std::string> parents;
    public:
    Commit():timestamp("Thu Jan 01 00:00:00 1970 +0000"),message("Initial Commit"){};
    ~Commit()=default;
    void save();//把commit给记录到文件夹中
    Commit load(std::string commit_id);//从文件夹中获得commit
    //方法属于类而不是成员
    static std::string genID(Commit commit);//生成commitid
    static std::string getParents(Commit commit);//记录父commit
};
#endif // COMMIT_H 
