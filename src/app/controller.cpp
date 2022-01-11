#include <app/controller.hpp>
#include <GLFW/glfw3.h>

namespace jk {
namespace {
float seekTime(dibs::Event::Key const& key) noexcept {
	float const dir = key.key == GLFW_KEY_LEFT ? -1.0f : 1.0f;
	float coeff = 5.0f;
	if (key.mods & GLFW_MOD_CONTROL) {
		coeff = 30.0f;
	} else if (key.mods & GLFW_MOD_SHIFT) {
		coeff = 1.0f;
	}
	return coeff * dir;
}
} // namespace

void Controller::onKey(dibs::Event::Key const& key) noexcept {
	if (key.action == GLFW_PRESS || key.action == GLFW_REPEAT) {
		float const magnitude = key.action == GLFW_REPEAT ? 0.01f : 0.05f;
		switch (key.key) {
		case GLFW_KEY_UP: add(Action::eVolume, magnitude); break;
		case GLFW_KEY_DOWN: add(Action::eVolume, -magnitude); break;
		default: break;
		}
	}
	if (key.action == GLFW_RELEASE) {
		switch (key.key) {
		case GLFW_KEY_SPACE:
		case GLFW_KEY_ENTER: add(Action::ePlayPause); break;
		case GLFW_KEY_ESCAPE: add(Action::eStop); break;
		case GLFW_KEY_LEFT:
		case GLFW_KEY_RIGHT: add(Action::eSeek, seekTime(key)); break;
		case GLFW_KEY_M: add(Action::eMute); break;
		default: break;
		}
		if (key.mods & GLFW_MOD_CONTROL) {
			switch (key.key) {
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
