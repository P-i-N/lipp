#include <lipp/lipp.hpp>

//---------------------------------------------------------------------------------------------------------------------
int main()
{
	using traits = lipp::preprocessor_traits<std::string, std::string_view, std::vector>;

	lipp::preprocessor<traits> pp;

	pp.include_file( "include_test.txt" );

	printf( "%s", pp.read_all().c_str() );
	return 0;
}
