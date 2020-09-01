#include <lipp/lipp.hpp>

#line 1 "Test.cpp"

//---------------------------------------------------------------------------------------------------------------------
int main()
{
	using traits = lipp::preprocessor_traits<char, std::string, std::string_view, std::vector>;

	lipp::preprocessor<traits> pp;

	pp.include_file( "whitespace.txt" );

	printf( "%s", pp.read_all().c_str() );
	return 0;
}
