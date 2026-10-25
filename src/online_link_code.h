#pragma once
#include "online_transport.h"
#include <winhttp.h>

namespace online {
inline uint32_t link_crc(const uint8_t* data,size_t size){
 uint32_t crc=~0u;for(size_t i=0;i<size;i++){crc^=data[i];for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&uint32_t(-int(crc&1)));}return ~crc;
}
inline std::string make_link_code(const std::string& endpoint,const Token& token){
 sockaddr_in address={};Token checked={};
 if(!parse_address(endpoint,address)||!token_from_text(token_text(token),checked))return {};
 std::vector<uint8_t> bytes(4);memcpy(bytes.data(),&address.sin_addr.s_addr,4);
 unsigned port=ntohs(address.sin_port);bytes.push_back(uint8_t(port>>8));bytes.push_back(uint8_t(port));
 bytes.insert(bytes.end(),token.begin(),token.end());put32(bytes,link_crc(bytes.data(),bytes.size()));
 constexpr char alphabet[]="0123456789ABCDEFGHJKMNPQRSTVWXYZ";
 std::string out="BALDR1-";unsigned bits=0,pending=0,chars=0;
 for(uint8_t b:bytes){pending=(pending<<8)|b;bits+=8;while(bits>=5){bits-=5;if(chars&&chars%7==0)out+='-';out+=alphabet[(pending>>bits)&31];chars++;}}
 if(bits){if(chars&&chars%7==0)out+='-';out+=alphabet[(pending<<(5-bits))&31];}return out;
}
inline bool parse_link_code(std::string text,std::string& endpoint,Token& token){
 if(text.size()>96)return false;std::string compact;
 for(char c:text){if(c=='-'||c==' '||c=='\r'||c=='\n'||c=='\t')continue;if(c>='a'&&c<='z')c=char(c-'a'+'A');compact+=c;}
 if(compact.size()!=48||compact.substr(0,6)!="BALDR1")return false;
 constexpr char alphabet[]="0123456789ABCDEFGHJKMNPQRSTVWXYZ";
 std::vector<uint8_t> bytes;unsigned pending=0,bits=0;
 for(size_t i=6;i<compact.size();i++){
  auto p=strchr(alphabet,compact[i]);if(!p||p-alphabet>=32)return false;pending=(pending<<5)|unsigned(p-alphabet);bits+=5;
  if(bits>=8){bits-=8;bytes.push_back(uint8_t(pending>>bits));}
 }
 if(bytes.size()!=26||(bits&&(pending&((1u<<bits)-1)))||get32(bytes.data()+22)!=link_crc(bytes.data(),22))return false;
 std::string host=std::to_string(bytes[0])+"."+std::to_string(bytes[1])+"."+std::to_string(bytes[2])+"."+std::to_string(bytes[3]);
 host+=":"+std::to_string(unsigned(bytes[4])*256+bytes[5]);sockaddr_in address={};
 Token key;memcpy(key.data(),bytes.data()+6,16);Token checked;
 if(!parse_address(host,address)||!token_from_text(token_text(key),checked))return false;
 endpoint=host;token=key;return true;
}
// Only invoked when an Internet host explicitly creates a code. No discovery
// service is needed to decode or join; the code carries the IPv4 endpoint.
inline std::string public_ipv4(){
 HINTERNET session=WinHttpOpen(L"BaldrOnline/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
 if(!session)return {};WinHttpSetTimeouts(session,3000,3000,3000,3000);
 HINTERNET connection=WinHttpConnect(session,L"api.ipify.org",INTERNET_DEFAULT_HTTPS_PORT,0);
 HINTERNET request=connection?WinHttpOpenRequest(connection,L"GET",L"/",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr;
 std::string answer;
 if(request&&WinHttpSendRequest(request,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(request,nullptr)){
  DWORD code=0,len=sizeof(code);WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&len,WINHTTP_NO_HEADER_INDEX);
  char buffer[65];DWORD n=0;if(code==200&&WinHttpReadData(request,buffer,64,&n)&&n&&n<64)answer.assign(buffer,n);
 }
 if(request)WinHttpCloseHandle(request);if(connection)WinHttpCloseHandle(connection);WinHttpCloseHandle(session);
 sockaddr_in address={};if(!parse_address(answer+":1",address))return {};return answer;
}
}
