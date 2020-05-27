#include <lipp/lipp.hpp>

//---------------------------------------------------------------------------------------------------------------------
int main()
{
	using traits = lipp::preprocessor_traits<std::string, std::string_view, std::vector>;

	lipp::preprocessor<traits> pp;

	pp.include_file( "eval.txt" );

	printf( "%s", pp.output().c_str() );
	return 0;
}
