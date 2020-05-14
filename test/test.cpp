#include <lipp/lipp.hpp>

#include <string>
#include <fstream>
#include <memory>

//---------------------------------------------------------------------------------------------------------------------
std::string read_file_as_string( const std::string &fileName )
{
	std::ifstream ifs( fileName, std::ios_base::in | std::ios_base::binary );
	if ( !ifs.is_open() )
		return std::string();

	ifs.seekg( 0, std::ios_base::end );
	size_t len = ifs.tellg();
	ifs.seekg( 0, std::ios_base::beg );

	std::unique_ptr<char[]> data( new char[len + 1] );
	ifs.read( data.get(), len );
	data[len] = 0;

	return data.get();
}

//---------------------------------------------------------------------------------------------------------------------
int main()
{
	lipp::preprocessor pp;

	auto str = read_file_as_string( "test.txt" );

	pp.process( str.c_str() );

	return 0;
}
