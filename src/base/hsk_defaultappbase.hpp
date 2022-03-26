#pragma once

#include "hsk_minimalappbase.hpp"

namespace hsk
{
	/// @brief Intended as base class for demo applications. Compared to MinimalAppBase it offers a complete simple vulkan setup.
	class DefaultAppBase : public MinimalAppBase
	{
	public:
		DefaultAppBase();
		~DefaultAppBase();

	protected:

	};
}