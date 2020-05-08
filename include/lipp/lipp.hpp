#pragma once

#if !defined(LIPP_USE_CUSTOM_TYPES)
#include <string>
#include <vector>
namespace lipp {
using string = std::string;
template <typename T> using vector = std::vector<T>;
} // namespace lipp
#else
/* Typedef your custom string & vector types with STL-like interfaces */
#endif

namespace lipp {

struct source
{
	string directory = ".";
	string text = "";
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using include_callback = source ( * )( const char *, bool );
using output_callback = void( * )( const char * );

namespace detail {

inline void printf_output( const char *str ) { printf( "%s\n", str ); }

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct preprocessor_params
{
	// Semicolon separated defines with or without values ("FOO=1;BAR;FLAGS=0xFFFF")
	string defines = "";

	// Source
	source src;

	// Callback to execute on #include directive
	include_callback include_cb = nullptr;

	// Callback to execute when writting a line out
	output_callback output_cb = detail::printf_output;
};

//---------------------------------------------------------------------------------------------------------------------
inline bool preprocess( preprocessor_params params )
{
	return true;
}

} // namespace lipp
