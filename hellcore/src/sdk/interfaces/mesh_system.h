#pragma once

class c_scene_animatable_object;
class c_animatable_scene_object_desc;
class c_cs_player_pawn;
class c_mesh_system {
public:
	c_scene_animatable_object* create_scene_animatable_object( c_cs_player_pawn* pawn );
};