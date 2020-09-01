#include <lipp/lipp.hpp>

#include "magic_enum.hpp"

//---------------------------------------------------------------------------------------------------------------------
std::string escape( std::string_view str ) LIPP_NOEXCEPT
{
	std::string result = "";

	for ( size_t i = 0, S = lipp::size( str ); i < S; ++i )
	{
		auto ch = str[i];
		/**/ if ( ch == '\t' )
			result += "\\t";
		else if ( ch == '\r' )
			result += "\\r";
		else if ( ch == '\n' )
			result += "\\n";
		else if ( ch == '"' )
			result += "\\\"";
		else if ( ch == '\\' )
			result += "\\";
		else if ( ch == 0 )
			result += "\\0";
		else
			result += ch;
	}

	return result;
}

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
		        escape( t.whitespace ).c_str(),
		        escape( t.text ).c_str() );
	}

	return 0;
}
