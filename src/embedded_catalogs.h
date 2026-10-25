#pragma once
// Small roster/stage/music name lists are embedded in the online EXE. Original
// game assets and equipment descriptions are not bundled in these resources.
static bool embedded_choices(unsigned id,std::vector<Choice>& choices){
 HMODULE module=GetModuleHandleW(nullptr);HRSRC entry=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
 if(!entry)return false;DWORD size=SizeofResource(module,entry);if(!size||size>65536)return false;
 HGLOBAL resource=LoadResource(module,entry);const char* bytes=resource?(const char*)LockResource(resource):nullptr;if(!bytes)return false;
 std::istringstream input(std::string(bytes,size));std::string line;std::vector<Choice> loaded;
 try{while(std::getline(input,line)){
  auto tab=line.find('\t');if(tab==std::string::npos)return false;
  std::string number=line.substr(0,tab);size_t used=0;unsigned value=std::stoul(number,&used);
  if(used!=number.size())return false;std::string label=line.substr(tab+1);if(!label.empty()&&label.back()=='\r')label.pop_back();
  if(label.empty()||(id==201&&!valid_mecha(value))||(id==202&&(value>220||value==9))||(id==203&&value>=92))return false;
  for(auto& prior:loaded)if(prior.id==value)return false;
  loaded.push_back({value,label+"  ["+std::to_string(value)+"]"});
 }}catch(...){return false;}
 if(loaded.empty())return false;choices.insert(choices.end(),loaded.begin(),loaded.end());return true;
}
