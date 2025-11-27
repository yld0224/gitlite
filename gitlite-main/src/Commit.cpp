#include "../include/Commit.h"
#include "../include/Utils.h"
#include <filesystem>
#include <sstream>
std::string Commit::genID(){
   std::ostringstream contents;
   contents<<"timestamp:"<<this->timestamp<<"\n";
   contents<<"message:"<<this->message<<"\n";
   for(auto&[filename,blob_id]: this->tracked_files){
    contents<<"tracked_files:"<<filename<<' '<<blob_id<<"\n";
   }
   for(auto parent:parents){
    contents<<"parents:"<<parent<<"\n";
   }
   return Utils::sha1(contents.str());
}
std::string Commit::getID(){
    return this->commit_id;
}
void Commit::save(){
   std::ostringstream contents;
   contents<<"timestamp:"<<this->timestamp<<"\n";
   contents<<"message:"<<this->message<<"\n";
   for(auto&[filename,blob_id]: this->tracked_files){
    contents<<"tracked_files:"<<filename<<' '<<blob_id<<"\n";
   }
   for(auto parent:parents){
    contents<<"parents:"<<parent<<"\n";
   }
   if(this->commit_id.empty()){
    this->commit_id=genID();
   }
   std::string cwd=static_cast<std::string>(std::filesystem::current_path());
   std::string path=Utils::join(cwd,".gitlite","obj",this->commit_id);
   Utils::writeContents(path,contents.str());
}