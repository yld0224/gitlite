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
    static Commit load(std::string commit_id);//从文件夹中获得commit
    std::string genID();//生成commitid
    std::string getID();
    std::vector<std::string> getParents(){
        return this->parents;
    }
    std::map<std::string,std::string> getTrackedFiles(){
        return this->tracked_files;
    }
};
#endif // COMMIT_H 
