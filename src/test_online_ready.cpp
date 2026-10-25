#include "online_chat.h"
#include <cassert>
#include <cstdio>

using namespace online;
static ControlPacket change(uint32_t version,bool value){ControlPacket p{Kind::Ready,{}};put32(p.bytes,version);put32(p.bytes,value?1:0);return p;}
static std::optional<ControlPacket> deliver(ReadyCheck& to,const ControlPacket& p){return to.receive(p.kind,p.bytes.data(),p.bytes.size());}
int main(){
 ReadyCheck host(0),guest(1);assert(!host.set_ready(true)&&!host.next_packet(0));host.connected(true);guest.connected(true);
 assert(!host.snapshot().localReady&&!guest.snapshot().peerReady);
 assert(host.set_ready(true)&&!host.set_ready(false));auto first=host.next_packet(0);assert(first&&first->kind==Kind::Ready);
 assert(!host.next_packet(249));auto retry=host.next_packet(250);assert(retry&&retry->bytes==first->bytes);
 auto ack=deliver(guest,*retry);assert(ack&&guest.snapshot().peerReady);deliver(host,*ack);
 assert(!host.next_packet(500)&&!guest.next_packet(500)); // One checkbox never starts.
 assert(host.set_ready(false));auto unready=host.next_packet(501);ack=deliver(guest,*unready);deliver(host,*ack);
 assert(!guest.snapshot().peerReady&&!host.snapshot().pending);
 ack=deliver(guest,*first);assert(ack&&!guest.snapshot().peerReady); // Reordered old ready cannot restore it.
 auto malformed=change(3,true);malformed.bytes[4]=2;assert(!deliver(guest,malformed));
 auto gap=change(4,true);assert(!deliver(guest,gap));
 assert(host.set_ready(true));auto current=host.next_packet(502);ack=deliver(guest,*current);deliver(host,*ack);
 assert(guest.set_ready(true));auto guestReady=guest.next_packet(503);auto lostAck=deliver(host,*guestReady);
 assert(lostAck&&guest.snapshot().pending);auto start=host.next_packet(504);assert(start&&start->kind==Kind::Start);
 assert(host.snapshot().starting&&!host.snapshot().started&&!host.set_ready(false));
 // A guest may uncheck before a delayed host proposal arrives. Reject that
 // proposal; the host cancels it after receiving the newer ready revision.
 deliver(guest,*lostAck);assert(guest.set_ready(false));assert(!deliver(guest,*start));
 auto guestOff=guest.next_packet(505);ack=deliver(host,*guestOff);deliver(guest,*ack);
 assert(!host.snapshot().starting&&!host.next_packet(506));assert(!deliver(host,ControlPacket{Kind::StartAck,start->bytes}));
 assert(guest.set_ready(true));guestReady=guest.next_packet(507);lostAck=deliver(host,*guestReady);
 start=host.next_packet(508);assert(start&&start->kind==Kind::Start);
 // Start proves that the host received the latest ready change, even if its
 // separate ReadyAck was lost. Lose StartAck too, then recover through retry.
 auto startAck=deliver(guest,*start);assert(startAck&&guest.snapshot().started&&!guest.snapshot().pending);
 assert(!host.snapshot().started&&!guest.set_ready(false)&&!host.next_packet(757));
 retry=host.next_packet(758);assert(retry&&retry->bytes==start->bytes);startAck=deliver(guest,*retry);deliver(host,*startAck);
 assert(host.snapshot().started&&!host.next_packet(2000));
 host.connected(false);guest.connected(false);assert(!host.snapshot().localReady&&!guest.snapshot().started&&!deliver(guest,*start));
 host.connected(true);guest.connected(true);assert(!host.snapshot().localReady&&!host.snapshot().peerReady&&!host.next_packet(3000));
 // Only the host can propose a start, with exactly the two current revisions.
 ReadyCheck wrongRole(0);wrongRole.connected(true);assert(!deliver(wrongRole,*start));
 ReadyCheck bounded(1);bounded.connected(true);
 for(size_t size=0;size<8;size++)assert(!bounded.receive(Kind::Ready,first->bytes.data(),size));
 assert(!bounded.receive(Kind::Ready,nullptr,8));
 ChatMailbox a(0,"Kou"),b(1,"Masa");a.peer_name("Masa");b.peer_name("Kou");a.connected(true);b.connected(true);
 assert(a.set_ready(true));auto packet=a.next_lobby_packet(0);auto response=b.receive_lobby(packet->kind,packet->bytes.data(),packet->bytes.size());
 assert(response);b.receive_lobby(packet->kind,packet->bytes.data(),packet->bytes.size());
 assert(a.snapshot().lines.size()==1&&b.snapshot().lines.size()==1&&b.snapshot().lines[0].text=="Kou is ready.");
 a.receive_lobby(response->kind,response->bytes.data(),response->bytes.size());
 assert(b.set_ready(true));packet=b.next_lobby_packet(1);response=a.receive_lobby(packet->kind,packet->bytes.data(),packet->bytes.size());
 packet=a.next_lobby_packet(2);response=b.receive_lobby(packet->kind,packet->bytes.data(),packet->bytes.size());
 b.receive_lobby(packet->kind,packet->bytes.data(),packet->bytes.size());a.receive_lobby(response->kind,response->bytes.data(),response->bytes.size());
 a.receive_lobby(response->kind,response->bytes.data(),response->bytes.size());
 for(auto mailbox:{&a,&b}){auto s=mailbox->snapshot();assert(s.ready.started&&s.lines.size()==3);
  assert(s.lines[0].system&&s.lines[0].text=="Kou is ready."&&s.lines[1].text=="Masa is ready.");
  assert(s.lines[2].text=="Both players are ready. Starting the netplay test.");
  // System notices and text have separate sequence spaces. A text ACK must
  // update its own line rather than the earlier notice with the same ID.
  assert(mailbox->send_text("after lobby"));auto text=mailbox->next_packet(1000);std::vector<uint8_t> textAck;put32(textAck,get32(text.data()));
  mailbox->acknowledge(textAck.data(),textAck.size());s=mailbox->snapshot();assert(!s.lines.back().system&&s.lines.back().delivery==ChatDelivery::Delivered);
  mailbox->connected(false);assert(!mailbox->snapshot().ready.localReady&&!mailbox->snapshot().ready.peerReady);
 }
 puts("PASS: both-ready gate, unready/stale revisions, malformed controls, lost ready/start ACKs, disconnect reset and exactly-once chat notices.");
}
