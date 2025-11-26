#ifndef COMMIT_H
#define COMMIT_H
#include <memory>
#include <string>
#include <ctime>
#include <unordered_map>
class Commit{
    private:
    std::string commit_time;
    std::string commit_message;
    std::string commit_id;
    std::vector<std::string> blob_id;
    std::string parent_id;
    public:
    Commit():commit_time("123"),commit_message("Initial Commit"){};
    ~Commit()=default;
};
#endif // COMMIT_H
