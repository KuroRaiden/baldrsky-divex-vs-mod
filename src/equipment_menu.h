#pragma once
static int equipment_editor_player=-1;
static unsigned equipment_editor_mecha=0;
static EquipmentSlots equipment_draft;
static char equipment_preset_name[65]={};
static std::string equipment_description;
static void open_equipment_editor(unsigned player,unsigned mecha){
 equipment_editor_player=(int)player;equipment_editor_mecha=mecha;
 equipment_draft=equipment_profiles.get(player,mecha,equipment_catalog);equipment_description.clear();equipment_preset_name[0]=0;
 ImGui::OpenPopup("Equipment");
}
static void equipment_editor(float scale){
 ImGui::SetNextWindowSize(ImVec2(590*scale,420*scale),ImGuiCond_Always);
 if(!ImGui::BeginPopupModal("Equipment",nullptr,ImGuiWindowFlags_NoResize))return;
 auto found=equipment_catalog.find(equipment_editor_mecha);
 if(found==equipment_catalog.end()){ImGui::TextUnformatted("Load the equipment catalog first.");if(ImGui::Button("Close"))ImGui::CloseCurrentPopup();ImGui::EndPopup();return;}
 auto& mecha=found->second;
 std::string label="Mecha "+std::to_string(equipment_editor_mecha);
 for(auto& choice:mechas)if(choice.id==equipment_editor_mecha)label=choice.label;
 ImGui::Text("%uP — %s",equipment_editor_player+1,label.c_str());
 ImGui::TextUnformatted("Choosing an equipped move moves it to this slot.");
 if(ImGui::BeginTable("loadout",3,ImGuiTableFlags_ScrollY|ImGuiTableFlags_RowBg|ImGuiTableFlags_BordersInnerH,ImVec2(0,205*scale))){
  ImGui::TableSetupColumn("Button",ImGuiTableColumnFlags_WidthFixed,50*scale);
  ImGui::TableSetupColumn("Range",ImGuiTableColumnFlags_WidthFixed,95*scale);
  ImGui::TableSetupColumn("Move");ImGui::TableSetupScrollFreeze(0,1);ImGui::TableHeadersRow();
  const char* ranges[]={"Melee","Dash","Mid range","Long range"};
  for(unsigned slot=0;slot<14;slot++){
   ImGui::PushID(slot);ImGui::TableNextRow();ImGui::TableNextColumn();
   if(slot<12)ImGui::Text("%c",'A'+slot/4);else ImGui::Text("FC %u",slot-11);
   ImGui::TableNextColumn();ImGui::TextUnformatted(slot<12?ranges[slot%4]:"Force Crash");ImGui::TableNextColumn();
   const auto* selected=equipment_move(mecha,equipment_draft[slot]);ImGui::SetNextItemWidth(-1);
   if(ImGui::BeginCombo("##move",selected?selected->name.c_str():"Empty")){
    if(ImGui::Selectable("Empty",equipment_draft[slot]==equipment_empty)){equipment_draft[slot]=equipment_empty;equipment_description.clear();}
    for(auto& move:mecha.moves)if(equipment_type_allowed(slot,move.type)){
     bool current=equipment_draft[slot]==move.id;
     ImGui::PushID(move.id);
     if(ImGui::Selectable(move.name.c_str(),current)){
      for(unsigned other=0;other<14;other++)if(other!=slot&&equipment_draft[other]==move.id)equipment_draft[other]=equipment_empty;
      equipment_draft[slot]=move.id;equipment_description=move.description;
     }
     if(ImGui::IsItemHovered()){equipment_description=move.description;ImGui::SetTooltip("%s",move.description.c_str());}
     if(current)ImGui::SetItemDefaultFocus();ImGui::PopID();
    }ImGui::EndCombo();
   }
   if(ImGui::IsItemHovered()&&selected)equipment_description=selected->description;
   ImGui::PopID();
  }ImGui::EndTable();
 }
 ImGui::BeginChild("description",ImVec2(0,42*scale));ImGui::TextWrapped("%s",equipment_description.empty()?"Hover a move to read its original game description.":equipment_description.c_str());ImGui::EndChild();
 ImGui::SetNextItemWidth(200*scale);
 if(ImGui::BeginCombo("##presets","Load a saved preset")){
  for(unsigned i=0;i<equipment_profiles.presets.size();i++){
   auto& preset=equipment_profiles.presets[i];if(preset.player!=(unsigned)equipment_editor_player||preset.mecha!=equipment_editor_mecha)continue;
   ImGui::PushID(i);if(ImGui::Selectable(preset.name.c_str()))equipment_draft=preset.slots;ImGui::PopID();
  }ImGui::EndCombo();
 }
 ImGui::SameLine();ImGui::SetNextItemWidth(170*scale);ImGui::InputTextWithHint("##name","Preset name",equipment_preset_name,sizeof(equipment_preset_name));
 ImGui::SameLine();
 if(ImGui::Button("Save preset")&&equipment_valid(mecha,equipment_draft)){
  std::string name=equipment_preset_name;for(char& c:name)if(c=='\t'||c=='\r'||c=='\n')c=' ';
  if(!name.empty()){
   bool replaced=false;for(auto& p:equipment_profiles.presets)if(p.player==(unsigned)equipment_editor_player&&p.mecha==equipment_editor_mecha&&p.name==name){p.slots=equipment_draft;replaced=true;}
   if(!replaced)equipment_profiles.presets.push_back({(unsigned)equipment_editor_player,equipment_editor_mecha,name,equipment_draft});
   if(!save_equipment_profiles())status="Could not save loadout presets.";
  }
 }
 if(ImGui::Button("Restore defaults"))equipment_draft=mecha.defaults;
 ImGui::SameLine();
 if(ImGui::Button("Apply")){
  if(equipment_valid(mecha,equipment_draft)){
   equipment_profiles.active[{(unsigned)equipment_editor_player,equipment_editor_mecha}]=equipment_draft;
   if(save_equipment_profiles()){status="Equipment saved for this player and mecha.";ImGui::CloseCurrentPopup();}else status="Could not save equipment.";
  }
 }
 ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
}
