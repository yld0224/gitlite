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
void Commit::setID(std::string new_id){
   this->commit_id=new_id;
   return;
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
   std::string path=Utils::join(cwd,".gitlite","objects",this->commit_id);
   Utils::writeContents(path,contents.str());
}
Commit Commit::load(std::string commit_id){
   std::string cwd=static_cast<std::string>(std::filesystem::current_path());
   std::string path=Utils::join(cwd,".gitlite","objects",commit_id);
   std::stringstream ss;
   ss<<Utils::readContentsAsString(path);
   std::string lines;
   Commit loading_commit;
   while(std::getline(ss,lines)){
      auto pos=lines.find(':');
      if(lines.substr(0,pos)=="timestamp"){
         loading_commit.timestamp=lines.substr(pos+1);
      }if(lines.substr(0,pos)=="message"){
         loading_commit.message=lines.substr(pos+1);
      }if(lines.substr(0,pos)=="parents"){
         loading_commit.parents.push_back(lines.substr(pos+1));
      }if(lines.substr(0,pos)=="tracked_files"){
         auto space_pos=lines.find(' ');
         std::string filename=lines.substr(pos+1,space_pos-pos-1);
         std::string blob_id=lines.substr(space_pos+1);
         loading_commit.tracked_files[filename]=blob_id;
      }
   }
   loading_commit.commit_id=commit_id;
   return loading_commit;
}
void Commit::showCommitInfo(){
   std::cout<<"==="<<std::endl;
   std::cout<<"commit "<<this->commit_id<<std::endl;
   if(this->parents.size()==2){
      std::string id1=this->parents[0].substr(0,7);
      std::string id2=this->parents[1].substr(0,7);
      std::cout<<"Merge: "<<id1<<' '<<id2<<std::endl;
   }
   std::cout<<"Date: "<<this->timestamp<<std::endl;
   std::cout<<this->message<<std::endl;
   std::cout<<std::endl;
}