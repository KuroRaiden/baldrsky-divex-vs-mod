#include "online_chat.h"
#include <cassert>
#include <cstdio>

int main(){
 using namespace online;
 assert(valid_chat_text("Hello, 100% ###"));assert(valid_chat_text("\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf"));
 assert(valid_chat_text("\xf0\x9f\x91\x8b"));assert(valid_chat_text(std::string(512,'x')));
 for(auto s:{""," ","line\nbreak","\t","\x7f","\x80","\xc0\x80","\xe0\x80\x80","\xed\xa0\x80","\xf4\x90\x80\x80","\xe3\x81"})assert(!valid_chat_text(s));
 assert(!valid_chat_text(std::string("a\0b",3))&&!valid_chat_text(std::string(513,'x')));
 ChatMailbox a(0),b(1);assert(!a.send_text("offline"));a.connected(true);b.connected(true);
 assert(a.send_text("first")&&a.send_text("second"));auto first=a.next_packet(0);assert(get32(first.data())==1);
 assert(a.next_packet(249).empty());assert(a.next_packet(250)==first); // Lost datagram, retry.
 assert(b.receive(first.data(),first.size())==1);assert(b.receive(first.data(),first.size())==1); // Lost ACK, no duplicate line.
 assert(b.snapshot().lines.size()==1&&b.snapshot().lines[0].text=="first");
 std::vector<uint8_t> ack;put32(ack,2);a.acknowledge(ack.data(),ack.size());assert(a.next_packet(500)==first); // Ignore future ACK.
 ack.clear();put32(ack,1);a.acknowledge(ack.data(),ack.size());auto second=a.next_packet(501);assert(get32(second.data())==2);
 auto gap=second;gap[0]=3;assert(!b.receive(gap.data(),gap.size()));assert(b.receive(second.data(),second.size())==2);
 ack.clear();put32(ack,2);a.acknowledge(ack.data(),ack.size());assert(a.next_packet(1000).empty());
 auto state=a.snapshot();assert(state.lines.size()==2&&state.lines[0].delivery==ChatDelivery::Delivered&&state.lines[1].delivery==ChatDelivery::Delivered);
 auto malformed=first;malformed[0]=0;assert(!b.receive(malformed.data(),malformed.size()));
 malformed=first;malformed[4]=0;assert(!b.receive(malformed.data(),malformed.size()));
 assert(!b.receive(first.data(),4));assert(!b.receive(nullptr,10));
 for(unsigned i=0;i<chat_queue_limit;i++)assert(a.send_text("queued"));assert(!a.send_text("overflow"));
 a.connected(false);state=a.snapshot();assert(!state.connected&&a.next_packet(2000).empty());
 for(auto& line:state.lines)if(line.text=="queued")assert(line.delivery==ChatDelivery::Failed);
 ChatMailbox receiver(0);receiver.connected(true);
 for(unsigned i=1;i<=105;i++){std::vector<uint8_t> bytes;put32(bytes,i);bytes.push_back('x');assert(receiver.receive(bytes.data(),bytes.size())==i);}
 assert(receiver.snapshot().lines.size()==chat_history_limit&&receiver.snapshot().lines.front().id==6);
 puts("PASS: chat UTF-8/length bounds, FIFO delivery, loss/retry/duplicate ACKs, queue/history limits and disconnect state.");
}
