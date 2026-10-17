#pragma once

#include <cstdint> 
#include <string>  
#include <limits>  
#include <math/math types/vector.h>

enum e_protobuf_bits_t : uint32_t {
    protoslot_1 = 1 << 0,
    protoslot_2 = 1 << 1,
    protoslot_3 = 1 << 2,
    protoslot_4 = 1 << 3,
    protoslot_5 = 1 << 4,
    protoslot_6 = 1 << 5,
    protoslot_7 = 1 << 6,
    protoslot_8 = 1 << 7,
    protoslot_9 = 1 << 8,
    protoslot_10 = 1 << 9,
    protoslot_11 = 1 << 10,
    protoslot_12 = 1 << 11,
    protoslot_13 = 1 << 12,
    protoslot_14 = 1 << 13,
    protoslot_15 = 1 << 14,
    protoslot_16 = 1 << 15,
    protoslot_17 = 1 << 16,
    protoslot_18 = 1 << 17
};

template <typename T>
struct repeated_ptr_field_t {
    uint64_t unk_field_;
    int current_size_;
    int capacity_;
    struct inner_container {
        int max_;
        T** elements() {
            return reinterpret_cast<T**>(reinterpret_cast<uintptr_t>(this) + 0x8);
        }
    }*container;
public:
    bool valid() const {
        if (current_size_ < 0 || capacity_ < 0 || current_size_ > capacity_ || capacity_ > 4096)
            return false;
        if (!container)
            return current_size_ == 0 && capacity_ == 0;
        return container->max_ >= current_size_ && container->max_ <= capacity_;
    }

    inline int& size() {
        return current_size_;
    }

    inline int& capacity() {
        return capacity_;
    }

    inline int& max_size() {
        return container->max_;
    }

    inline void clear() {
 if (!valid()) return;
 if(container) for(int i=0;i<current_size_;++i) {
  auto* message=container->elements()[i];
  if(message) g_memory->call_virtual<void>(message,3);
 }
 current_size_=0;
}

    T* add(T* element) {
        using fn_add_to_container = T * (__fastcall*)(repeated_ptr_field_t*, T*);
        static auto fn = g_modules->m_client.find(xx("48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 48 8B FA 48 8B 49 ? 48 85 C9 74 ? 8B 01")).as<fn_add_to_container>();
        return fn(this, element);
    }

    inline uint64_t unk_field() {
        return unk_field_;
    }

    inline T*& operator[](int i) {
        return container->elements()[i];
    }

    inline operator bool() {
        return container != nullptr;
    }
};

class c_base_pb {
public:
    void* m_vtable;
    std::uintptr_t m_arena_metadata;
    uint32_t m_has_bits;
    int32_t m_cached_size;

    void set_bits( uint64_t n_bits ) {
        m_has_bits |= static_cast<uint32_t>(n_bits);
    }
};

static_assert(sizeof( c_base_pb ) == 0x18);

class c_msg_q_angle : public c_base_pb {
public:
    vec3_t m_ang_value;

    void set_q_angle( const vec3_t& ang_value ) {
        m_ang_value = ang_value;
        set_bits( e_protobuf_bits_t::protoslot_1 | e_protobuf_bits_t::protoslot_2 | e_protobuf_bits_t::protoslot_3 );
    }
};

static_assert(sizeof( c_msg_q_angle ) == 0x28);

class c_msg_vector : public c_base_pb {
public:
    vec3_t m_vec_value;

    void set_vector( const vec3_t& vec_value ) {
        m_vec_value = vec_value;
        set_bits( e_protobuf_bits_t::protoslot_1 | e_protobuf_bits_t::protoslot_2 | e_protobuf_bits_t::protoslot_3 );
    }
};
static_assert(sizeof( c_msg_vector ) == 0x28);

class c_csgo_interpolation_info_pb_cl : public c_base_pb {
public:
    float m_fraction;

    void set_fraction( const float& fl_fraction ) {
        m_fraction = fl_fraction;
        set_bits( e_protobuf_bits_t::protoslot_1 );
    };
};
static_assert(sizeof( c_csgo_interpolation_info_pb_cl ) == 0x20);

class c_csgo_interpolation_info_pb : public c_base_pb {
public:
    float m_fraction;
    int m_src_tick;
    int m_dst_tick;

    void set_fraction( const float& fl_fraction ) {
        m_fraction = fl_fraction;
        set_bits( e_protobuf_bits_t::protoslot_1 );
    }

    void set_src_tick( const int& i_src_tick ) {
        m_src_tick = i_src_tick;
        set_bits( e_protobuf_bits_t::protoslot_2 );
    }

    void set_dst_tick( const int& i_dst_tick ) {
        m_dst_tick = i_dst_tick;
        set_bits( e_protobuf_bits_t::protoslot_3 );
    }
};
static_assert(sizeof( c_csgo_interpolation_info_pb ) == 0x28);

class c_csgo_input_history_entry_pb : public c_base_pb {
public:
    c_msg_q_angle* m_view_angles;
    c_csgo_interpolation_info_pb_cl* m_cl_interp;
    c_csgo_interpolation_info_pb* m_sv_interp0;
    c_csgo_interpolation_info_pb* m_sv_interp1;
    c_csgo_interpolation_info_pb* m_player_interp;
    c_msg_vector* m_shoot_position;
    c_msg_vector* m_target_head_position_check;
    c_msg_vector* m_target_abs_position_check;
    c_msg_q_angle* m_target_ang_position_check;
    int m_render_tick_count;
    float m_render_tick_fraction;
    int m_player_tick_count;
    float m_player_tick_fraction;
    int m_frame_number;
    int m_target_ent_index;

    void set_render_tick_count( const int& i_render_tick_count ) {
        m_render_tick_count = i_render_tick_count;
        set_bits( e_protobuf_bits_t::protoslot_10 );
    }

    void set_render_tick_fraction( const float& fl_render_tick_fraction ) {
        m_render_tick_fraction = fl_render_tick_fraction;
        set_bits( e_protobuf_bits_t::protoslot_11 );
    }

    void set_player_tick_count( const int& i_player_tick_count ) {
        m_player_tick_count = i_player_tick_count;
        set_bits( e_protobuf_bits_t::protoslot_12 );
    }

    void set_player_tick_fraction( const float& fl_player_tick_fraction ) {
        m_player_tick_fraction = fl_player_tick_fraction;
        set_bits( e_protobuf_bits_t::protoslot_13 );
    }

    void set_frame_number( const int& i_frame_number ) {
        m_frame_number = i_frame_number;
        set_bits( e_protobuf_bits_t::protoslot_14 );
    }

    void set_target_ent_index( const int& i_target_ent_index ) {
        m_target_ent_index = i_target_ent_index;
        set_bits( e_protobuf_bits_t::protoslot_15 );
    }
};
static_assert(sizeof( c_csgo_input_history_entry_pb ) == 0x78);

struct in_button_state_pb_t : c_base_pb {
    uint64_t m_value;
    uint64_t m_value_changed;
    uint64_t m_value_scroll;

    void set_value( const uint64_t& u_value ) {
        m_value = u_value;
        set_bits( e_protobuf_bits_t::protoslot_1 );
    }

    void set_value_changed( const uint64_t& u_value_changed ) {
        m_value_changed = u_value_changed;
        set_bits( e_protobuf_bits_t::protoslot_2 );
    }

    void set_value_scroll( const uint64_t& u_value_scroll ) {
        m_value_scroll = u_value_scroll;
        set_bits( e_protobuf_bits_t::protoslot_3 );
    }
};

static_assert(sizeof( in_button_state_pb_t ) == 0x30);

struct subtick_move_step_t : c_base_pb {
public:
    uint64_t m_button;
    bool m_pressed;
    float m_when;
    float m_analog_forward_delta;
    float m_analog_left_delta;
    float m_analog_pitch_delta;
    float m_analog_yaw_delta;

    void set_button( const uint64_t& button ) {
        m_button = button;
        set_bits( e_protobuf_bits_t::protoslot_1 );
    }

    void set_pressed( const bool& pressed ) {
        m_pressed = pressed;
        set_bits( e_protobuf_bits_t::protoslot_2 );
    }

    void set_when( const float& when ) {
        m_when = when;
        set_bits( e_protobuf_bits_t::protoslot_3 );
    }

    void set_analog_forward_delta( const float& forward_delta ) {
        m_analog_forward_delta = forward_delta;
        set_bits( e_protobuf_bits_t::protoslot_4 );
    }

    void set_analog_left_delta( const float& analog_left_delta ) {
        m_analog_left_delta = analog_left_delta;
        set_bits( e_protobuf_bits_t::protoslot_5 );
    }

    void set_analog_pitch_delta( const float& analog_pitch_delta ) {
        m_analog_pitch_delta = analog_pitch_delta;
        set_bits( e_protobuf_bits_t::protoslot_6 );
    }

    void set_analog_yaw_delta( const float& analog_yaw_delta ) {
        m_analog_yaw_delta = analog_yaw_delta;
        set_bits( e_protobuf_bits_t::protoslot_7 );
    }
};
static_assert(sizeof( subtick_move_step_t ) == 56);

struct c_base_user_cmd_execution_notes {
    std::string* ignored_reason;
};

class c_base_user_cmd_pb : public c_base_pb {
public:
    repeated_ptr_field_t<subtick_move_step_t> m_subtick_moves_field;
    std::string* m_move_crc;
    in_button_state_pb_t* m_in_button_state;
    c_msg_q_angle* m_view_angles;
    c_base_user_cmd_execution_notes* m_execution_notes;
    int m_legacy_command_number;
    int m_client_tick;
    float m_forward_move;
    float m_side_move;
    float m_up_move;
    int m_impulse;
    int m_weapon_select;
    int m_random_seed;
    int m_moused_x;
    int m_moused_y;
    unsigned int m_prediction_offset_ticks_x256;
    unsigned int m_consumed_server_angle_changes;
    int m_cmd_flags;
    unsigned int m_pawn_entity_handle;

    subtick_move_step_t* create_subtick_move()
    {
        if (!m_subtick_moves_field.valid())
            return nullptr;
        if (m_subtick_moves_field && m_subtick_moves_field.size() < m_subtick_moves_field.max_size())
            return m_subtick_moves_field[m_subtick_moves_field.size()++];

        using fn_create_subtick_move = subtick_move_step_t * (__fastcall*)(uint64_t);
        static auto fn = g_modules->m_client.find(xx("E8 ? ? ? ? 48 8B D0 49 8D 4E ? E8 ? ? ? ? 4C 8B C8 4C 8D 43 ? 49 8B D1 48 8B CE E8 ? ? ? ? 48 8B D8 48 85 C0 0F 84 ? ? ? ? 48 3B 06 0F 83 ? ? ? ? 0F B7 00 66 C7 45 ? ? ? 66 3B 45 ? 74 ? E9 ? ? ? ? 40 80 FF ? 0F 85 ? ? ? ? 41 83 4E")).relative(0x1, 0x5).as<fn_create_subtick_move>();

        auto subtick_move = fn(m_subtick_moves_field.unk_field());
        if (!subtick_move)
            return nullptr;
        m_subtick_moves_field.add(subtick_move);

        return subtick_move;
    }

    void set_legacy_command_number( const int& n_command_number ) {
        m_legacy_command_number = n_command_number;
        set_bits( e_protobuf_bits_t::protoslot_5 );
    }

    void set_client_tick( const int& n_client_tick ) {
        m_client_tick = n_client_tick;
        set_bits( e_protobuf_bits_t::protoslot_6 );
    }

    void set_forward_move( const float& fl_forward_move ) {
        m_forward_move = fl_forward_move;
        set_bits( e_protobuf_bits_t::protoslot_7 );
    }

    void set_side_move( const float& fl_side_move ) {
        m_side_move = fl_side_move;
        set_bits( e_protobuf_bits_t::protoslot_8 );
    }

    void set_up_move( const float& fl_up_move ) {
        m_up_move = fl_up_move;
        set_bits( e_protobuf_bits_t::protoslot_9 );
    }

    void set_impulse( const int& n_impulse ) {
        m_impulse = n_impulse;
        set_bits( e_protobuf_bits_t::protoslot_10 );
    }

    void set_weapon_select( const int& n_weapon_select ) {
        m_weapon_select = n_weapon_select;
        set_bits( e_protobuf_bits_t::protoslot_11 );
    }

    void set_random_seed( const int& n_random_seed ) {
        m_random_seed = n_random_seed;
        set_bits( e_protobuf_bits_t::protoslot_12 );
    }

    void set_moused_x( const int& n_moused_x ) {
        m_moused_x = n_moused_x;
        set_bits( e_protobuf_bits_t::protoslot_13 );
    }

    void set_moused_y( const int& n_moused_y ) {
        m_moused_y = n_moused_y;
        set_bits( e_protobuf_bits_t::protoslot_14 );
    }

    void set_prediction_offset_ticks_x256(const uint32_t& value) {
        m_prediction_offset_ticks_x256 = value;
        set_bits(e_protobuf_bits_t::protoslot_15);
    }

    void set_consumed_server_angle_changes( const uint32_t& u_consumed_server_angle_changes ) {
        m_consumed_server_angle_changes = u_consumed_server_angle_changes;
        set_bits( e_protobuf_bits_t::protoslot_16 );
    }

    void set_cmd_flags( const int& n_flags ) {
        m_cmd_flags = n_flags;
        set_bits( e_protobuf_bits_t::protoslot_17 );
    }

    void set_pawn_entity_handle( const uint32_t& u_handle ) {
        m_pawn_entity_handle = u_handle;
        set_bits( e_protobuf_bits_t::protoslot_18 );
    }

    void clear_subtick_buttons( ) {
        for ( int i = 0; i < m_subtick_moves_field.size( ); i++ ) {
            subtick_move_step_t* p_sub_tick = m_subtick_moves_field[i];
            if ( !p_sub_tick )
                continue;
            p_sub_tick->set_button( 0 );
            p_sub_tick->set_pressed( false );
        }
    }

    c_msg_q_angle* get_view_angles() {
        set_bits(e_protobuf_bits_t::protoslot_3);
        if (m_view_angles)
            return m_view_angles;

        using fn_create_new_msg_angle = c_msg_q_angle * (__fastcall*)(void*);
        static auto fn = g_modules->m_client.find(xx("48 89 5C 24 ? 57 48 83 EC ? 33 DB 48 8B F9 48 85 C9 75 ? B9 ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 45 33 C0 33 D2 48 8B C8 E8 ? ? ? ? 48 8B D8 48 8B C3 48 8B 5C 24 ? 48 83 C4 ? 5F C3 4C 8D 05 ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 45 33 C0 48 8B D7 48 8B C8 E8 ? ? ? ? 48 8B 5C 24 ? 48 83 C4 ? 5F C3 CC CC CC CC CC 48 89 5C 24 ? 57 48 83 EC ? 33 DB 48 8B F9 48 85 C9 75 ? B9 ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 45 33 C0 33 D2 48 8B C8 E8 ? ? ? ? 48 8B D8 48 8B C3 48 8B 5C 24 ? 48 83 C4 ? 5F C3 4C 8D 05 ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 45 33 C0 48 8B D7 48 8B C8 E8 ? ? ? ? 48 8B 5C 24 ? 48 83 C4 ? 5F C3 CC CC CC CC CC 48 89 5C 24 ? 57 48 83 EC ? 33 FF 48 8B D9 48 85 C9 75 ? B9 ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 48 89 78 ? EB ? 48 8B C7 48 8B 5C 24 ? 48 83 C4 ? 5F C3 4C 8D 05 ? ? ? ? BA ? ? ? ? E8 ? ? ? ? 48 89 58 ? 48 8B 5C 24 ? 48 8D 0D ? ? ? ? 48 89 08 33 C9 48 89 48 ? 48 89 78 ? 48 89 78 ? 48 83 C4 ? 5F C3 CC 48 89 5C 24 ? 57 48 83 EC ? 33 DB")).as<fn_create_new_msg_angle>();

        auto arena = m_arena_metadata & ~std::uintptr_t{3};
        if (m_arena_metadata & 1)
            arena = *reinterpret_cast<std::uintptr_t*>(arena);
        m_view_angles = fn(reinterpret_cast<void*>(arena));
        set_bits(e_protobuf_bits_t::protoslot_3);

        return m_view_angles;
    }
};
static_assert(sizeof( c_base_user_cmd_pb ) == 0x88);

class c_csgo_user_cmd_pb {
public:
    void* __vfptr;
    std::uintptr_t m_arena_metadata;
    uint32_t m_has_bits;
    int32_t m_cached_size;
    repeated_ptr_field_t<c_csgo_input_history_entry_pb> m_input_history_field;
    c_base_user_cmd_pb* m_base_cmd; // 0x28
    bool m_left_hand_desired;
    bool m_is_predicting_body_shot_fx;
    bool m_is_predicting_head_shot_fx;
    bool m_is_predicting_kill_ragdolls;
    //int32_t m_attack3_start_history_index;
    int32_t m_attack1_start_history_index;
    int32_t m_attack2_start_history_index;

    void set_bits( uint64_t n_bits ) {
        m_has_bits |= static_cast<uint32_t>(n_bits);
    }

    

    void set_left_hand_desired( const bool& b_desired ) {
        m_left_hand_desired = b_desired;
        set_bits( e_protobuf_bits_t::protoslot_2 );
    }

    void set_is_predicting_body_shot( const bool& b_desired ) {
        m_is_predicting_body_shot_fx = b_desired;
        set_bits( e_protobuf_bits_t::protoslot_3 );
    }

    void set_is_predicting_head_shot( const bool& b_desired ) {
        m_is_predicting_head_shot_fx = b_desired;
        set_bits( e_protobuf_bits_t::protoslot_4 );
    }

    void set_is_prediction_kill_ragdoll( const bool& b_desired ) {
        m_is_predicting_kill_ragdolls = b_desired;
        set_bits( e_protobuf_bits_t::protoslot_5 );
    }

    void set_attack1_history_index( const int& n_index ) {
        m_attack1_start_history_index = n_index;
        set_bits( e_protobuf_bits_t::protoslot_6 );
    }

    void set_attack2_history_index( const int& n_index ) {
        m_attack2_start_history_index = n_index;
        set_bits( e_protobuf_bits_t::protoslot_7 );
    }

   /* void set_attack3_history_index( const int& n_index ) {
        m_attack3_start_history_index = n_index;
        set_bits( e_protobuf_bits_t::protoslot_6 );
    }*/
};
static_assert(sizeof( c_csgo_user_cmd_pb ) == 0x48);

struct in_button_state_t {
public:
    void* m_vtable;
    uint64_t m_value;
    uint64_t m_value_changed;
    uint64_t m_value_scroll;

    enum e_button_state : int8_t {
        in_button_up = 0,
        in_button_down = 1,
        in_button_down_up = 2,
        in_button_up_down = 3,
        in_button_up_down_up = 4,
        in_button_down_up_down = 5,
        in_button_down_up_down_up = 6,
        in_button_up_down_up_down = 7
    };

    void set_button_state( const uint64_t& u_value, e_button_state e_button_state ) {
        switch ( e_button_state ) {
        case e_button_state::in_button_up: {
            m_value &= ~u_value;
            m_value_changed &= ~u_value;
            m_value_scroll &= ~u_value;
            break;
        }
        case e_button_state::in_button_down: {
            m_value |= u_value;
            m_value_changed &= ~u_value;
            m_value_scroll &= ~u_value;
            break;
        }
        case e_button_state::in_button_down_up: {
            m_value &= ~u_value;
            m_value_changed |= u_value;
            m_value_scroll &= ~u_value;
            break;
        }
        case e_button_state::in_button_up_down: {
            m_value |= u_value;
            m_value_changed |= u_value;
            m_value_scroll &= ~u_value;
            break;
        }
        case e_button_state::in_button_up_down_up: {
            m_value &= ~u_value;
            m_value_changed &= ~u_value;
            m_value_scroll |= u_value;
            break;
        }
        case e_button_state::in_button_down_up_down: {
            m_value |= u_value;
            m_value_changed &= ~u_value;
            m_value_scroll |= u_value;
            break;
        }
        case e_button_state::in_button_down_up_down_up: {
            m_value &= ~u_value;
            m_value_changed |= u_value;
            m_value_scroll |= u_value;
            break;
        }
        case e_button_state::in_button_up_down_up_down: {
            m_value |= u_value;
            m_value_changed |= u_value;
            m_value_scroll |= u_value;
            break;
        }
        }
    }
};
static_assert(sizeof( in_button_state_t ) == 0x20);

class c_user_cmd {
public:
    void* m_vtable;
    int m_sequence_number;
    //char pad0[0xC];
    c_csgo_user_cmd_pb m_pb; // 0x18
    in_button_state_t m_buttons;
    char pad1[8];
    double m_real_time;
    float m_curtime;
    bool m_has_been_predicted;
    int m_previous_flags;
    int m_current_flags;
};
static_assert(sizeof( c_user_cmd ) == 0x98);
static_assert(offsetof(c_base_pb, m_arena_metadata) == 0x8);
static_assert(offsetof(c_base_pb, m_has_bits) == 0x10);
static_assert(offsetof(c_base_pb, m_cached_size) == 0x14);
static_assert(offsetof(c_base_user_cmd_pb, m_view_angles) == 0x40);
static_assert(offsetof(c_base_user_cmd_pb, m_client_tick) == 0x54);
static_assert(offsetof(c_csgo_user_cmd_pb, m_base_cmd) == 0x30);
static_assert(offsetof(c_csgo_user_cmd_pb, m_attack1_start_history_index) == 0x3C);
static_assert(offsetof(c_user_cmd, m_pb) == 0x10);
static_assert(offsetof(c_user_cmd, m_buttons) == 0x58);
