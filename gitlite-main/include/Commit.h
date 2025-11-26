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
    Commit():timestamp("123"),message("Initial Commit"){};
    ~Commit()=default;
    void save();
    Commit load(std::string commit_id);
};
#endif // COMMIT_H 
