#include "trace.h"

#include <core/interfaces/interfaces.h>
#include <utils/memory.h>

int c_game_trace::get_hitbox_id() const
{
    if (m_hitbox)
        return m_hitbox->m_hitbox_id;

    return 0;
}

int c_game_trace::get_hit_group() const
{
    if (m_hitbox)
        return m_hitbox->m_hit_group;

    return 0;
}

bool c_game_trace::hit_world() const {
    auto world_entity = g_interfaces->m_entity_system->get_base_entity(0).as<void*>();
    return m_ent == world_entity;
}

bool c_game_trace::is_visible() const {
    return (m_fraction > 0.97f);
}

void c_vphys2world::init_game_trace(c_game_trace* game_trace)
{
    using fn_init_game_trace = void(__fastcall*)(c_game_trace*);
    static auto fn = g_modules->m_client.find(xx("40 55 41 55 41 57 48 83 EC")).as<fn_init_game_trace>();
    fn(game_trace);
}



void c_vphys2world::init_player_movement_trace_filter(c_trace_filter* trace_filter, void* entity_instance, uint64_t interact_with, int collision_group)
{
    using func_t = void(__fastcall*)(c_trace_filter*, void*, uint64_t, int);
    static func_t fn = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF C7 41")).as<func_t>();

    return fn(trace_filter, entity_instance, interact_with, collision_group);
}

void c_vphys2world::setup_trace_data(c_game_trace* game_trace, c_ray* ray, vec3_t start, vec3_t end, c_game_trace_data* trace_data)
{
    game_trace->m_ray_type = ray->m_type;
    if (trace_data->m_fraction >= 1.0 || !trace_data->m_start_in_solid && end.x == 0.0 && end.y == 0.0 && end.z == 0.0)
    {
        init_game_trace(game_trace);

        game_trace->m_start_pos = start;
        game_trace->m_end_pos = (start + end);
    }
    else
    {
        auto interpolated_end_pos = vec3_t((end.x * trace_data->m_fraction) + start.x, (end.y * trace_data->m_fraction) + start.y, (end.z * trace_data->m_fraction) + start.z);
        game_trace->m_end_pos = interpolated_end_pos;

        game_trace->m_hit_normal = trace_data->m_hit_normal;
        game_trace->m_hit_offset = trace_data->m_hit_offset;
        game_trace->m_fraction = trace_data->m_fraction;
        game_trace->m_start_in_solid = trace_data->m_start_in_solid;

        bool shit = 0;
        c_transform* transform = trace_data->m_body->get_body_transform(&shit);
        vec3_t hit_point = trace_data->m_hit_point;
        game_trace->m_body_transform = *transform;

        game_trace->m_hit_point = hit_point;
        if (!ray->m_type || ray->m_type == 1 && (game_trace->m_shape->get_shape_type() == 3))
            game_trace->m_exact_hit_point = true;

        //game_trace->m_surface_prop = trace_data->m_surface_property;
        game_trace->m_shape_attributes = *trace_data->m_shape->get_collision_attribute();
        game_trace->m_shape_attributes.m_entity_id = trace_data->m_body->get_entity_id();
        game_trace->m_contents = trace_data->m_shape->get_collision_attribute()->m_interacts_as;

        c_cs_player_pawn* hit_entity = g_interfaces->m_entity_system->get_base_entity(trace_data->m_body->get_entity_id()).as< c_cs_player_pawn* >();
        game_trace->m_ent = hit_entity;
        game_trace->m_start_pos = start;
        game_trace->m_triangle = trace_data->m_triangle;
    }
}

bool c_vphys2world::clip_ray_entity(c_ray* ray, vec3_t start, vec3_t end, c_cs_player_pawn* pawn, c_trace_filter* filter, c_game_trace* game_trace) {
 if(!pawn || !filter || !game_trace) return false;
 return trace_shape(ray,&start,&end,filter,game_trace) && game_trace->m_ent==pawn;
}

void c_vphys2world::setup_game_trace_info(c_trace_data* trace_data, c_game_trace* game_trace, float unk, c_segment_holder* trace_holder) {
    using fn = void(__fastcall*)(c_trace_data*, c_game_trace*, float, c_segment_holder*);
    static fn x = g_modules->m_client.find(xx("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B E9 0F 29 74 24")).as<fn>();
    if (!x)
        return;
    x(trace_data, game_trace, unk, trace_holder);
}

void c_vphys2world::trace_to_exit(c_trace_data* trace, vec3_t start, vec3_t end, c_trace_filter& filter, int penetration_count)
{
    using fn_trace_to_exit = void(__fastcall*)(c_trace_data*, const vec3_t&, const vec3_t&, const c_trace_filter&, int, bool);
    static auto fn = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 40 F2 0F 10 02 4D 8D 71 08")).as<fn_trace_to_exit>();
    trace->m_trace_segments_ptr = reinterpret_cast<c_segment_holder*>(trace->m_trace_segments);
    trace->m_trace_info = reinterpret_cast<c_trace_info*>(trace->m_hit_elements);
    if (fn) fn(trace, start, end, filter, penetration_count, true);
}

void c_vphys2world::get_trace_info(c_trace_data* data, c_game_trace* trace, float fl, PVOID unk) {
    using fn = void(__fastcall*)(c_trace_data*, c_game_trace*, float, PVOID);
    static fn x = g_modules->m_client.find(xx("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B E9 0F 29 74 24")).as<fn>();

    return x(data, trace, fl, unk);
}

void c_vphys2world::get_trace_info(c_trace_data* trace, c_game_trace* hit, c_trace_info* trace_info)
{
    using func_t = void(__fastcall*)(c_trace_data*, c_game_trace*, c_trace_info*);
    static auto fn = g_modules->m_client.find(xx("41 0F B7 40 ? F3 41 0F 10 10")).as<func_t>();
    return fn(trace, hit, trace_info);
}

void c_vphys2world::init_trace(c_trace_filter& filter, c_cs_player_pawn* pawn, uint64_t mask, uint8_t layer, uint16_t) {
 filter = c_trace_filter(mask,pawn,nullptr,layer);
}

bool c_vphys2world::trace_shape(c_ray* ray, vec3_t* start, vec3_t* end, c_trace_filter* filter, c_game_trace* game_trace)
{
    using fn_trace_shape = bool (__fastcall*)(c_vphys2world*, c_ray*, vec3_t*, vec3_t*, c_trace_filter*, c_game_trace*);
    static auto fn = g_modules->m_client.find(xx("48 89 54 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? 00 00")).as<fn_trace_shape>();
    if (!fn)
        return false;
    return fn(this, ray, start, end, filter, game_trace);
}





c_trace_filter* c_vphys2world::initialize_trace_filter(c_trace_filter* filter, c_cs_player_pawn* pawn, int64_t mask, uint8_t layer, uint8_t layer2) {
    using fn = c_trace_filter*(__fastcall*)(c_trace_filter*, c_cs_player_pawn*, int64_t, uint8_t, uint8_t);
    static fn x = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF C7 41 ?")).as<fn>();
    if (!x)
        return filter;
    return x(filter, pawn, mask, layer, layer2);
}

c_trace_data* c_vphys2world::initialize_trace_data(c_trace_data* data) {
    using fn = c_trace_data * (__fastcall*)(c_trace_data*);
    static fn x = g_modules->m_client.find(xx("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 79 ? 33 F6 C7 47")).as<fn>();
    if (!x)
        return data;
    return x(data);
}

void c_vphys2world::update_world_collisions()
{
    g_memory->call_virtual<void>(this, 263);
}