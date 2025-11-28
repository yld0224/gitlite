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
    Commit(const Commit &other){
        timestamp=other.timestamp;
        message=other.message;
        commit_id=other.commit_id;
        tracked_files=other.tracked_files;
        parents=other.parents;
    };
    ~Commit()=default;
    void save();//把commit给记录到文件夹中
    static Commit load(std::string commit_id);//从文件夹中获得commit
    std::string genID();//生成commitid
    std::string getID();
    void setID(std::string);
    std::vector<std::string> getParents(){
        return this->parents;
    }
    std::vector<std::string> setParents(std::string parent_id){
        this->parents.clear();
        this->parents.push_back(parent_id);
        return this->parents;
    }
    std::map<std::string,std::string> getTrackedFiles(){
        return this->tracked_files;
    }
    void setTrackedFiles(Commit& commit,std::map<std::string,std::string>& files){
        commit.tracked_files=files;
    }
    void setTime(){
        std::time_t currentTime = std::time(nullptr);
        std::tm* localTime = std::localtime(&currentTime);
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y %z", localTime);
        this->timestamp = std::string(buffer);
        return;
    }
    void setMessage(std::string new_message){
        this->message=new_message;
        return;
    }
};
#endif // COMMIT_H 
