#include "equipment_profiles.h"
#include <cassert>
#include <cstdio>
int main(){
 EquipmentMecha mech;mech.defaults.fill(equipment_empty);
 mech.moves={{0,0,"Punch","Native description"},{1,0,"Kick","Kick description"},{2,1,"FC","FC description"},{3,2,"Support FC",""}};
 mech.defaults[0]=0;mech.defaults[1]=1;mech.defaults[12]=2;mech.defaults[13]=3;
 assert(equipment_valid(mech,mech.defaults));
 auto invalid=mech.defaults;invalid[2]=0;assert(!equipment_valid(mech,invalid));
 invalid=mech.defaults;invalid[0]=2;invalid[12]=equipment_empty;assert(!equipment_valid(mech,invalid));
 invalid=mech.defaults;invalid[12]=0;invalid[0]=equipment_empty;assert(!equipment_valid(mech,invalid));
 invalid=mech.defaults;invalid[0]=99;assert(!equipment_valid(mech,invalid));
 for(unsigned slot=0;slot<12;slot++)assert((equipment_save_slot(slot)%3)*4+equipment_save_slot(slot)/3==slot);
 EquipmentCatalog catalog={{0,mech}};EquipmentProfiles profiles;
 auto alternate=mech.defaults;std::swap(alternate[0],alternate[1]);
 profiles.active[{0,0}]=mech.defaults;profiles.active[{1,0}]=alternate;
 profiles.presets.push_back({1,0,"Second player",alternate});
 std::stringstream file;profiles.write(file);EquipmentProfiles loaded;assert(loaded.read(file,catalog));
 assert(loaded.get(0,0,catalog)==mech.defaults);assert(loaded.get(1,0,catalog)==alternate);assert(loaded.presets[0].slots==alternate);
 auto other=loaded.get(1,1,catalog);for(auto id:other)assert(id==equipment_default);
 std::stringstream broken("BALDR-PROFILES\t1\nA\t1\t0\t-\t999\n");assert(!loaded.read(broken,catalog));assert(loaded.get(1,0,catalog)==alternate);
 VSEquipmentConfig config;assert(equipment_shape_valid(config,0,1));config.players[1][0]=0;config.players[1][1]=0;assert(!equipment_shape_valid(config,0,1));
 puts("PASS: native slot mapping, move types, duplicates, unknown moves, independent mirror loadouts and preset persistence.");
}
