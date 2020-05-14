#pragma once

#if !defined(LIPP_USE_CUSTOM_TYPES)
#include <string>
#include <vector>
namespace lipp {
using string_t = std::string;
using string_view_t = std::string_view;
template <typename T> using vector_t = std::vector<T>;
} // namespace lipp
#else
/* Typedef your custom types with STL-like interfaces */
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace lipp {

class preprocessor
{
public:
	preprocessor();

	virtual ~preprocessor();

	virtual bool define( string_view_t name, string_view_t value );

	bool define( string_view_t name ) { return define( name, string_view_t( "", 0 ) ); }

	virtual bool undef( string_view_t name );

	virtual void undef_all();

	virtual const char *find_macro( string_view_t name ) const noexcept;

	struct source
	{
		string_t dir;
		string_t text;
	};

	bool process( const source &src );

	bool process( const char *str );

protected:
	virtual string_t on_include( string_view_t fileName, bool isSystemPath );

	virtual void on_output( string_view_t line )
	{
		printf( "%.*s\n", int( line.size() ), line.data() );
	}

	static string_view_t trim( string_view_t str, bool left = true, bool right = true ) noexcept;

	static string_view_t split_token( string_view_t &str ) noexcept;

	bool process_line( string_view_t line );

	bool if_stack_result() const noexcept;

	int process_directive( string_view_t dir );

	int evaluate_expression( string_view_t expr ) const;

	struct macro
	{
		string_t name;
		string_t value;
	};

	using macros_t = vector_t<macro>;

	macros_t _macros;

	bool _insideCommentBlock = false;

	vector_t<bool> _ifStack;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
inline preprocessor::preprocessor()
{

}

//---------------------------------------------------------------------------------------------------------------------
inline preprocessor::~preprocessor()
{

}

//---------------------------------------------------------------------------------------------------------------------
inline bool preprocessor::define( string_view_t name, string_view_t value )
{
	for ( auto &m : _macros )
	{
		if ( m.name == name )
		{
			m.value = value;
			return true;
		}
	}

	_macros.push_back( { string_t( name ), string_t( value ) } );
	return false;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool preprocessor::undef( string_view_t name )
{
	for ( size_t i = 0, S = _macros.size(); i < S; ++i )
	{
		if ( _macros[i].name == name )
		{
			_macros[i] = std::move( _macros[S - 1] );
			_macros.pop_back();
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------
inline void preprocessor::undef_all()
{
	_macros.clear();
}

//---------------------------------------------------------------------------------------------------------------------
inline const char *preprocessor::find_macro( string_view_t name ) const noexcept
{
	for ( const auto &m : _macros )
		if ( m.name == name )
			return m.value.c_str();

	return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool preprocessor::process( const source &src )
{
	const char *str = src.text.c_str();

	while ( str )
	{
		auto eol = strchr( str, '\n' );
		size_t length = eol ? ( eol - str ) : strlen( str );

		process_line( string_view_t( str, length ) );

		if ( str = eol )
			++str;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool preprocessor::process( const char *str )
{
	return process( source { ".", str } );
}

//---------------------------------------------------------------------------------------------------------------------
inline string_t preprocessor::on_include( string_view_t fileName, bool isSystemPath )
{
	string_t result = "";


	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline string_view_t preprocessor::trim( string_view_t str, bool left, bool right ) noexcept
{
	size_t start = 0;
	size_t end = str.size();

	if ( left )
	{
		while ( start < str.size() && isspace( str.data()[start] ) )
			++start;
	}

	if ( right )
	{
		while ( end > start && isspace( str.data()[end - 1] ) )
			--end;
	}

	return string_view_t( str.data() + start, end - start );
}

//---------------------------------------------------------------------------------------------------------------------
inline string_view_t preprocessor::split_token( string_view_t &str ) noexcept
{
	str = trim( str, true, false );

	if ( !str.size() )
		return string_view_t();

	static constexpr char *alphaNumChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";

	char firstChar = *str.data();
	size_t length = 0;

	if ( strchr( alphaNumChars, firstChar ) )
	{
		while ( strchr( alphaNumChars, str.data()[length] ) )
			++length;
	}
	else if ( strchr( "'\"", firstChar ) )
	{

	}
	else if ( strchr( "=()&|!><+-/*", firstChar ) )
	{

	}

	auto result = string_view_t( str.data(), length );
	str = string_view_t( str.data() + length, str.size() - length );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool preprocessor::process_line( string_view_t line )
{
	bool shouldOutput = true;
	size_t directivePos( -1 );

	char ch = 0, lastCh = 0;
	for ( size_t i = 0; i < line.size(); ++i, lastCh = ch )
	{
		ch = line.data()[i];
		if ( isspace( ch ) )
			continue;

		if ( _insideCommentBlock )
		{
			// End of comment block?
			if ( ch == '/' && lastCh == '*' )
				_insideCommentBlock = false;
			else
				continue;
		}

		// Start of comment block
		/**/ if ( ch == '*' && lastCh == '/' )
		{
			_insideCommentBlock = true;
		}
		// Line comment
		else if ( ch == '/' && lastCh == '/' )
		{
			break;
		}
		else if ( ch == '#' )
		{
			directivePos = i;
			break;
		}
	}

	if ( directivePos < line.size() )
	{
		++directivePos;

		if ( auto err = process_directive( string_view_t( line.data() + directivePos, line.size() - directivePos ) ); err < 0 )
			return err;
		else
			shouldOutput = ( err > 0 );
	}

	if ( shouldOutput && if_stack_result() )
		on_output( line );

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline bool preprocessor::if_stack_result() const noexcept
{
	for ( bool i : _ifStack )
		if ( !i )
			return false;

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
inline int preprocessor::process_directive( string_view_t dir )
{
	auto dirName = split_token( dir );

	/**/ if ( dirName == "define" )
	{
		if ( auto macroName = split_token( dir ); macroName.size() )
		{
			define( macroName, split_token( dir ) );
			return 1;
		}

		return -1; // Error!
	}
	else if ( dirName == "undef" )
	{
		if ( auto macroName = split_token( dir ); macroName.size() )
		{
			undef( macroName );
			return 1;
		}

		return -1; // Error!
	}
	else if ( dirName == "ifdef" )
	{
		if ( auto macroName = split_token( dir ); macroName.size() )
			_ifStack.push_back( find_macro( macroName ) != nullptr );
		else
			return -1; // Error!
	}
	else if ( dirName == "else" )
	{
		if ( !_ifStack.size() )
			return -1;

		auto lastIf = _ifStack[_ifStack.size() - 1];
		_ifStack[_ifStack.size() - 1] = !lastIf;
	}
	else if ( dirName == "endif" )
	{
		if ( !_ifStack.size() )
			return -1;

		_ifStack.pop_back();
	}
	else if ( dirName == "include" )
	{
		if ( dir = trim( dir ); dir.size() < 3 )
			return -1;

		auto firstCh = dir.data()[0];
		auto lastCh = dir.data()[dir.size() - 1];
		bool isSystemPath = false;

		if ( ( firstCh == '"' && lastCh == '"' ) || ( firstCh == '\'' && lastCh == '\'' ) )
			isSystemPath = false;
		else if ( firstCh == '<' && lastCh == '>' )
			isSystemPath = true;
		else
			return -1;

		auto fileName = string_view_t( dir.data() + 1, dir.size() - 2 );
		on_include( fileName, isSystemPath );
	}

	return 0; // Do not output
}

//---------------------------------------------------------------------------------------------------------------------
inline int preprocessor::evaluate_expression( string_view_t expr ) const
{
	return 0;
}

} // namespace lipp
