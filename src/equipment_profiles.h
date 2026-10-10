#pragma once
#include "equipment_rules.h"
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
struct EquipmentMove {unsigned id,type;std::string name,description;};
struct EquipmentMecha {EquipmentSlots defaults;std::vector<EquipmentMove> moves;};
using EquipmentCatalog=std::map<unsigned,EquipmentMecha>;
inline const EquipmentMove* equipment_move(const EquipmentMecha& mecha,unsigned id){for(auto& m:mecha.moves)if(m.id==id)return &m;return nullptr;}
inline bool equipment_valid(const EquipmentMecha& mecha,const EquipmentSlots& slots){
 if(!equipment_unique(slots))return false;
 for(unsigned i=0;i<14;i++)if(slots[i]!=equipment_empty){auto move=equipment_move(mecha,slots[i]);if(!move||!equipment_type_allowed(i,move->type))return false;}
 return true;
}
inline bool read_equipment_catalog(std::istream& input,EquipmentCatalog& catalog){
 std::string line;if(!std::getline(input,line)||line!="BALDR-EQUIPMENT\t1")return false;
 EquipmentCatalog result;
 try{while(std::getline(input,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();std::vector<std::string> fields;std::istringstream parts(line);std::string part;
  while(std::getline(parts,part,'\t'))fields.push_back(part);
  if(fields.empty())continue;
  if(fields[0]=="M"&&fields.size()==16){unsigned id=std::stoul(fields[1]);auto& entry=result[id];for(unsigned s=0;s<14;s++)entry.defaults[s]=std::stoul(fields[s+2]);}
  else if(fields[0]=="W"&&fields.size()>=5){unsigned id=std::stoul(fields[1]);result[id].moves.push_back({(unsigned)std::stoul(fields[2]),(unsigned)std::stoul(fields[3]),fields[4],fields.size()>5?fields[5]:""});}
  else return false;
 }}catch(...){return false;}
 if(result.empty())return false;
 for(auto& item:result)if(!equipment_valid(item.second,item.second.defaults))return false;
 catalog=std::move(result);return true;
}
struct EquipmentPreset {unsigned player,mecha;std::string name;EquipmentSlots slots;};
struct EquipmentProfiles {
 std::map<std::pair<unsigned,unsigned>,EquipmentSlots> active;
 std::vector<EquipmentPreset> presets;
 EquipmentSlots get(unsigned player,unsigned mecha,const EquipmentCatalog& catalog)const{
  auto entry=catalog.find(mecha);if(entry==catalog.end())return default_equipment();
  auto found=active.find({player,mecha});return found!=active.end()&&equipment_valid(entry->second,found->second)?found->second:entry->second.defaults;
 }
 bool read(std::istream& in,const EquipmentCatalog& catalog){
  std::string line;if(!std::getline(in,line)||line!="BALDR-PROFILES\t1")return false;EquipmentProfiles parsed;
  while(std::getline(in,line)){
   std::istringstream row(line);std::string kind,p,m,name,value;
   if(!std::getline(row,kind,'\t')||!std::getline(row,p,'\t')||!std::getline(row,m,'\t')||!std::getline(row,name,'\t'))return false;
   try{unsigned player=std::stoul(p),mecha=std::stoul(m);auto entry=catalog.find(mecha);EquipmentSlots slots;
    for(auto& id:slots){if(!std::getline(row,value,'\t'))return false;id=std::stoul(value);}
    if(player>1||entry==catalog.end()||!equipment_valid(entry->second,slots))return false;
    if(kind=="A")parsed.active[{player,mecha}]=slots;
    else if(kind=="P"&&!name.empty()&&name.size()<=64)parsed.presets.push_back({player,mecha,name,slots});else return false;
   }catch(...){return false;}
  }*this=std::move(parsed);return true;
 }
 void write(std::ostream& out)const{
  out<<"BALDR-PROFILES\t1\n";
  for(auto& a:active){out<<"A\t"<<a.first.first<<'\t'<<a.first.second<<"\t-";for(auto id:a.second)out<<'\t'<<id;out<<'\n';}
  for(auto& p:presets){out<<"P\t"<<p.player<<'\t'<<p.mecha<<'\t'<<p.name;for(auto id:p.slots)out<<'\t'<<id;out<<'\n';}
 }
};
