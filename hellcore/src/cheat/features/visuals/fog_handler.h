#pragma once

#include <sdk/entity/entity.h>

class c_fog_handler {
public:
	void fog_controller();
	void remove_fog();

private:
	c_base_entity* m_gradient_fog = nullptr;
};

inline auto g_fog_handler = std::make_unique<c_fog_handler>();
