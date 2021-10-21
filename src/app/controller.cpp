#include <app/controller.hpp>

namespace jk {
namespace {
float seekTime(Key const& key) noexcept {
	float const dir = key.is(GLFW_KEY_LEFT) ? -1.0f : 1.0f;
	float coeff = 5.0f;
	if (key.all(GLFW_MOD_CONTROL)) {
		coeff = 30.0f;
	} else if (key.all(GLFW_MOD_SHIFT)) {
		coeff = 1.0f;
	}
	return coeff * dir;
}
} // namespace

Controller::Controller(Input::Signal&& signal) : m_signal(std::move(signal)) {
	m_signal += [this](Key key) { onKey(key); };
}

void Controller::onKey(Key key) noexcept {
	if (key.release()) {
		switch (key.code) {
		case GLFW_KEY_SPACE:
		case GLFW_KEY_ENTER: add(Action::ePlayPause); break;
		case GLFW_KEY_ESCAPE: add(Action::eStop); break;
		case GLFW_KEY_LEFT:
		case GLFW_KEY_RIGHT: add(Action::eSeek, seekTime(key)); break;
		case GLFW_KEY_M: add(Action::eMute); break;
		case GLFW_KEY_UP: add(Action::eVolume, 0.1f); break;
		case GLFW_KEY_DOWN: add(Action::eVolume, -0.1f); break;
		default: break;
		}
		if (key.all(GLFW_MOD_CONTROL)) {
			switch (key.code) {
			case GLFW_KEY_P: add(Action::ePrev); break;
			case GLFW_KEY_N: add(Action::eNext); break;
			case GLFW_KEY_Q: add(Action::eQuit); break;
			default: break;
			}
		}
	}
}

void Controller::add(Action action, float value) noexcept {
	if (m_list.has_space()) { m_list.push_back({value, action}); }
}
} // namespace jk
