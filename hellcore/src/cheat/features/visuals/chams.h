#pragma once

#include <context.h>
#include <sdk/datatypes/scene_object.h>
#include <core/hooks/hooks.h>
#include <cheat/hooks/scene_system/scene_system_hooks.h>
#include <cheat/config/vars.h>
#include <cheat/features/entity cache/entity_cache.h>

#define REMOVE_ALL -1
#define SINGLE_OBJECT_INDEX 0

struct model_object_t {
    std::array<c_scene_animatable_object*, 12> m_objects;
    float m_draw_begin_time;
    float m_target_simtime;
    vec3_t m_impact_position;

    inline bool exists( c_scene_animatable_object* compare ) {
        for ( c_scene_animatable_object*& object : m_objects ) {
            if ( object == compare )
                return true;
        }
        return false;
    }

    bool create( c_cs_player_pawn* pawn, int object_index );
    void remove( int object_index );

    void set_bones( c_scene_animatable_object* object, bone_data_t* bones, int count );

    void handle( c_cs_player_pawn* pawn, bool onshot = false );
};

inline std::unordered_map< c_cs_player_pawn*, model_object_t > s_backtrack_models;
inline std::unordered_map< c_cs_player_pawn*, tight_array< model_object_t, 64 > > s_onshot_models;

class c_chams {
private:
    struct cham_material_t {
        c_strong_handle<c_material_2> m_visible;
        c_strong_handle<c_material_2> m_occluded;
    };
private:
    std::array<cham_material_t, e_materials::material_max> m_materials;
public:
    void init(void);

    void set_material_and_color(c_mesh_primitive* mesh_primitive, c_material_color color, c_material_2* material);
    bool on_generate_primitives(c_animatable_scene_object_desc* desc, c_scene_animatable_object* object, void* a3, c_mesh_primitive_output_buffer* render_buf);
    void prune_old_onshot(c_cs_player_pawn* pawn);
};
inline auto g_chams = std::make_unique<c_chams>();