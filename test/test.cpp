#include <lipp/lipp.hpp>

#include "magic_enum.hpp"

//---------------------------------------------------------------------------------------------------------------------
int main()
{
	using traits = lipp::preprocessor_traits<char, std::string, std::string_view, std::vector>;

	lipp::preprocessor<traits> pp;

	pp.include_file( "whitespace.txt" );

	lipp::preprocessor<traits>::token t;
	while ( pp.next_token( t ) )
	{
		auto nameSV = magic_enum::enum_name( t.type );
		printf( "token_type=%.*s, whitespace=\"%s\", text=\"%s\"\n",
		        int( nameSV.size() ), nameSV.data(),
		        lipp::escape<std::string>( t.whitespace ).c_str(),
		        lipp::escape<std::string>( t.text ).c_str() );
	}

	return 0;
}
