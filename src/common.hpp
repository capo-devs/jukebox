#pragma once

constexpr bool jk_debug =
#if defined(JK_DEBUG)
	true;
#else
	false;
#endif
