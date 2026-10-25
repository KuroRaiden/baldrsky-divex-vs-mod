#pragma once
#include "online_chat.h"
#include <istream>
#include <ostream>

namespace online {
constexpr size_t name_max_characters=24,name_max_bytes=96;
inline std::string trim_name(const std::string& name){
 auto first=name.find_first_not_of(' ');if(first==std::string::npos)return {};
 return name.substr(first,name.find_last_not_of(' ')-first+1);
}
inline bool valid_player_name(const std::string& name){
 if(name.size()>name_max_bytes||!valid_chat_text(name)||trim_name(name)!=name)return false;
 size_t count=0;for(uint8_t c:name)if((c&0xc0)!=0x80)count++;
 return count<=name_max_characters;
}
inline bool read_player_name(std::istream& in,std::string& name){
 std::string value;char c;
 while(in.get(c)){if(value.size()>name_max_bytes+2)return false;value+=c;}
 if(!value.empty()&&value.back()=='\n')value.pop_back();
 if(!value.empty()&&value.back()=='\r')value.pop_back();
 if(!valid_player_name(value))return false;name=value;return true;
}
inline bool write_player_name(std::ostream& out,const std::string& name){
 if(!valid_player_name(name))return false;out<<name<<'\n';return bool(out);
}
}
