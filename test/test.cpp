#include <lipp/lipp.hpp>

//---------------------------------------------------------------------------------------------------------------------
int main()
{
	using traits = lipp::preprocessor_traits<char, std::string, std::string_view, std::vector>;

	lipp::preprocessor<traits> pp;

	pp.include_file( "eval.txt" );

	printf( "%s", pp.read_all().c_str() );
	return 0;
}
