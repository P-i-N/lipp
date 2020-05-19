#include <lipp/lipp.hpp>

//---------------------------------------------------------------------------------------------------------------------
int main()
{
	using traits = lipp::preprocessor_traits<std::string, std::string_view, std::vector>;

	lipp::preprocessor<traits> pp;

	pp.include_file( "test.txt" );

	return 0;
}
