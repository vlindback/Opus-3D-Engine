#include <editor/include/editor_application.hpp>

namespace opus3d
{
	EditorApplication::EditorApplication(const engine::EngineConfig& engineConfig) : m_engine(engineConfig) {}

	EditorApplication::~EditorApplication() {}

} // namespace opus3d