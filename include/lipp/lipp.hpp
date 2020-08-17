#pragma once

#include <string.h>

#if !defined(LIPP_NOEXCEPT)
	#define LIPP_NOEXCEPT noexcept
#endif

#if !defined(LIPP_SPRINTF)
	#if defined(_MSC_VER)
		#define LIPP_SPRINTF(...) sprintf_s( __VA_ARGS__ );
	#else
		#define LIPP_SPRINTF(...) sprintf( __VA_ARGS__ );
	#endif
#endif

#if !defined(LIPP_DO_NOT_USE_STL)
#include <string>
#include <vector>
#include <fstream>

namespace lipp {

template <class T, class A>
void clear( std::vector<T, A> &vec ) LIPP_NOEXCEPT { vec.clear(); }

template <class T, class A>
void push_back( std::vector<T, A> &vec, const T &item ) LIPP_NOEXCEPT { vec.push_back( item ); }

template <class T, class A>
void pop_back( std::vector<T, A> &vec ) LIPP_NOEXCEPT { vec.pop_back(); }

template <class T, class A>
void swap_erase_at( std::vector<T, A> &vec, size_t index ) LIPP_NOEXCEPT
{ vec[index] = std::move( vec[vec.size() - 1] ); pop_back( vec ); }

} // namespace lipp
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace lipp {

enum class token_type
{
	unknown = 0,
	eof,
	number,
	identifier,
	string,
	directive,
	parent_left,
	parent_right,
	brace_left,
	brace_right,
	add,
	subtract,
	divide,
	multiply,
	assign,
	logical_and,
	logical_or,
	equal,
	not_equal,
	less_equal,
	greater_equal,
	logical_not,
	less,
	greater,
	semicolon,
};

enum class error_type
{
	none = 0,
	unexpected_eof,
	syntax_error,
	invalid_string,
	invalid_path,
	expected_identifier,
	mismatch_if,
	include_error,
	read_failed,
	expression_too_complex,
	invalid_expression,
};

struct parsing_flags
{
	enum
	{
		stop_at_eols     = 0b0'0000'0001,
		expand_macros    = 0b0'0000'0010,
		unescape_strings = 0b0'0000'0100,

		default_parsing_flags = expand_macros | unescape_strings
	};
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline size_t size( const char *str ) LIPP_NOEXCEPT { return str ? strlen( str ) : 0; }

template <class T>
inline size_t size( const T &container ) LIPP_NOEXCEPT { return container.size(); }

template <class T>
inline const T *data( const T &container ) LIPP_NOEXCEPT { return container.data(); }

template <class T>
inline auto substr( const T &str, size_t offset, size_t length ) LIPP_NOEXCEPT
{ return str.substr( offset, length ); }

template <class T>
inline auto substr( const T &str, size_t offset ) LIPP_NOEXCEPT
{ return str.substr( offset ); }

template <class T>
inline auto char_at( const T &str, size_t index ) LIPP_NOEXCEPT
{ return index < lipp::size( str ) ? str[index] : 0; }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class Char, class String, class StringView, template <class> class Vector>
struct preprocessor_traits
{
	using char_t = Char;
	using string_t = String;
	using string_view_t = StringView;
	template <typename T> using vector_t = Vector<T>;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
class preprocessor
{
public:
	using traits_t = T;
	using char_t = typename traits_t::char_t;
	using string_t = typename traits_t::string_t;
	using string_view_t = typename traits_t::string_view_t;
	template <typename U> using vector_t = typename traits_t::template vector_t<U>;

	preprocessor() = default;

	virtual ~preprocessor() = default;

	virtual bool define( string_view_t name, string_view_t value ) LIPP_NOEXCEPT;

	bool define( string_view_t name ) LIPP_NOEXCEPT { return define( name, "" ); }

	virtual bool undef( string_view_t name ) LIPP_NOEXCEPT;

	virtual const char_t *find_macro( string_view_t name ) const LIPP_NOEXCEPT;

	virtual void reset() LIPP_NOEXCEPT;

	virtual bool include_string( string_view_t src, string_view_t sourceName ) LIPP_NOEXCEPT;

	bool include_string( string_view_t src ) LIPP_NOEXCEPT { return include_string( src, "" ); }

	virtual bool include_file( string_view_t fileName, bool isSystemPath ) LIPP_NOEXCEPT;

	bool include_file( string_view_t fileName ) LIPP_NOEXCEPT { return include_file( fileName, false ); }

	bool is_inside_true_block() const LIPP_NOEXCEPT { return !( ( _ifBits + 1ull ) & _ifBits ); }

	string_view_t current_source_name() const LIPP_NOEXCEPT { return _sourceName; }

	int current_line_number() const LIPP_NOEXCEPT { return _lineNumber; }

	error_type error() const LIPP_NOEXCEPT { return _error; }

	struct token
	{
		token_type type;
		string_view_t whitespace;
		string_view_t text;
	};

	bool next_token( token &result, int flags = parsing_flags::default_parsing_flags ) LIPP_NOEXCEPT;

	string_t read_all() LIPP_NOEXCEPT;

protected:
	static constexpr size_t char_t_buffer_size = 256;
	static constexpr size_t expression_stack_size = 16;

	static string_view_t trim( string_view_t s ) LIPP_NOEXCEPT;

	bool parse_next_token( token &result, int flags = parsing_flags::default_parsing_flags ) LIPP_NOEXCEPT;

	virtual void set_error( error_type e ) LIPP_NOEXCEPT { _error = e; }

	virtual bool read_file( string_view_t fileName, string_t &output ) LIPP_NOEXCEPT;

	virtual int process_unknown_directive( string_view_t name ) LIPP_NOEXCEPT { return 1; }

	bool concat_remaining_tokens( string_t &result ) LIPP_NOEXCEPT;

	bool process_directive( token &result ) LIPP_NOEXCEPT;

	int evaluate_expression() LIPP_NOEXCEPT;

	struct macro
	{
		string_t name;
		string_t value;
	};

	using macros_t = vector_t<macro>;

	macros_t _macros;

	string_t _source = string_t();

	string_t _sourceName = string_t();

	string_view_t _cwd = string_view_t();

	string_t _tempString = string_t();

	size_t _cursor = 0;

	int _lineNumber = 0;

	error_type _error = error_type::none;

	unsigned long long _ifBits = 0;

	bool _insideCommentBlock = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::define( string_view_t name, string_view_t value ) LIPP_NOEXCEPT
{
	name = trim( name );
	value = trim( value );

	for ( auto &m : _macros )
	{
		if ( m.name == name )
		{
			m.value = value;
			return true;
		}
	}

	push_back( _macros, { string_t( name ), string_t( value ) } );
	return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::undef( string_view_t name ) LIPP_NOEXCEPT
{
	for ( size_t i = 0, S = lipp::size( _macros ); i < S; ++i )
	{
		if ( _macros[i].name == name )
		{
			swap_erase_at( _macros, i );
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline const typename T::char_t *preprocessor<T>::find_macro( string_view_t name ) const LIPP_NOEXCEPT
{
	for ( const auto &m : _macros )
		if ( m.name == name )
			return m.value.c_str();

	return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline void preprocessor<T>::reset() LIPP_NOEXCEPT
{
	clear( _macros );
	_source = string_t();
	_sourceName = string_t();
	_cwd = string_view_t();
	_tempString = string_t();
	_cursor = 0;
	_lineNumber = 0;
	_error = error_type::none;
	_ifBits = 0;
	_insideCommentBlock = false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::include_string( string_view_t src, string_view_t sourceName ) LIPP_NOEXCEPT
{
	if ( !lipp::size( src ) )
		return true;

	char_t buff[char_t_buffer_size] = { };

	LIPP_SPRINTF( buff, "#line 1 \"%.*s\"\n", int( lipp::size( sourceName ) ), data( sourceName ) );

	string_t source = buff;
	source += src;

	if ( source[lipp::size( source ) - 1] != '\n' )
		source += "\n";

	LIPP_SPRINTF( buff, "#line %d \"%s\"\n", _lineNumber + 1, data( _sourceName ) );
	source += buff;

	_source = substr( _source, 0, _cursor ) + source + substr( _source, _cursor );
	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::include_file( string_view_t fileName, bool isSystemPath ) LIPP_NOEXCEPT
{
	char_t buff[char_t_buffer_size] = { };

	if ( !isSystemPath && lipp::size( _cwd ) )
	{
		LIPP_SPRINTF( buff, "%.*s/%.*s",
		              int( lipp::size( _cwd ) ),
		              data( _cwd ),
		              int( lipp::size( fileName ) ),
		              data( fileName ) )
	}
	else
	{
		LIPP_SPRINTF( buff, "%.*s",
		              int( lipp::size( fileName ) ),
		              data( fileName ) )
	}

	// Correct path separators
	for ( size_t i = 0, S = lipp::size( buff ); i < S; ++i )
	{
		if ( buff[i] == '\\' )
			buff[i] = '/';
	}

	string_t fileContent;
	if ( !read_file( buff, fileContent ) )
	{
		if ( _error == error_type::none )
			_error = error_type::read_failed;

		return false;
	}

	return include_string( fileContent, buff );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::read_file( string_view_t fileName, string_t &output ) LIPP_NOEXCEPT
{
#if !defined(LIPP_DO_NOT_USE_STL)
	if ( std::ifstream ifs( string_t( fileName ).c_str() ); ifs.is_open() )
	{
		output = string_t( std::istreambuf_iterator<char_t>( ifs ), std::istreambuf_iterator<char_t>() );
		return true;
	}
#endif

	_error = error_type::read_failed;
	return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline
typename T::string_view_t preprocessor<T>::trim( string_view_t s ) LIPP_NOEXCEPT
{
	size_t start = 0;
	size_t end = lipp::size( s );

	while ( start < end && s[start] <= 32 ) ++start;
	while ( end > start && s[end - 1] <= 32 ) --end;

	return ( start < end ) ? lipp::substr( s, start, end - start ) : string_view_t();
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::next_token( token &result, int flags ) LIPP_NOEXCEPT
{
	result = token();

	while ( parse_next_token( result, flags ) )
	{
		if ( is_inside_true_block() )
			return true;
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::parse_next_token( token &result, int flags ) LIPP_NOEXCEPT
{
	auto src = lipp::substr( string_view_t( _source ), _cursor );

	size_t whitespaceLength = 0;
	bool insideLineComment = false;
	bool prevInsideCommentBlock = _insideCommentBlock;

	// Consume as much whitespace as possible
	while ( whitespaceLength < lipp::size( src ) )
	{
		auto ch = lipp::char_at( src, whitespaceLength );

		if ( ch == '\n' )
		{
			if ( !!( flags & parsing_flags::stop_at_eols ) )
			{
				_insideCommentBlock = prevInsideCommentBlock;
				return false;
			}

			++_lineNumber;
		}

		if ( _insideCommentBlock )
		{
			if ( ch == '*' && lipp::char_at( src, whitespaceLength + 1 ) == '/' )
			{
				_insideCommentBlock = false;
				++whitespaceLength;
			}
		}
		else if ( insideLineComment )
		{
			if ( ch == '\n' )
				insideLineComment = false;
		}
		else if ( ch == '/' )
		{
			if ( auto nextCh = lipp::char_at( src, whitespaceLength + 1 ); nextCh == '/' )
			{
				insideLineComment = true;
				++whitespaceLength;
			}
			else if ( nextCh == '*' ) // Start of block comment?
			{
				_insideCommentBlock = true;
				++whitespaceLength;
			}
			else
				break;
		}
		else if ( ch > 32 )
			break;

		++whitespaceLength;
	}

	if ( !!( flags & parsing_flags::stop_at_eols ) && whitespaceLength == lipp::size( src ) )
	{
		_insideCommentBlock = prevInsideCommentBlock;
		return false;
	}

	result.whitespace = lipp::substr( src, 0, whitespaceLength );
	src = lipp::substr( src, whitespaceLength );
	_cursor += whitespaceLength;

	if ( lipp::size( src ) == 0 )
	{
		if ( _insideCommentBlock )
		{
			_error = error_type::unexpected_eof;
			return false;
		}

		result.type = token_type::eof;
		return false;
	}

	static constexpr char_t *alphaChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";
	static constexpr char_t *numChars = "0123456789";
	static constexpr char_t *numDotChars = "0123456789.";

	size_t tokenLength = 1;

	/**/ if ( auto ch = lipp::char_at( src, 0 ); ch == '#' )
	{
		src = substr( src, 1 ); // Cut away '#'
		_cursor += 1;
		result.type = token_type::directive;
		return process_directive( result );
	}
	else if ( strchr( alphaChars, ch ) )
	{
		result.type = token_type::identifier;

		while ( tokenLength < lipp::size( src ) )
		{
			auto ch = lipp::char_at( src, tokenLength );
			if ( !strchr( alphaChars, ch ) && !strchr( numChars, ch ) )
				break;

			++tokenLength;
		}
	}
	else if ( strchr( numChars, ch ) ||
	          ( ( ch == '+' || ch == '-' ) && strchr( numDotChars, lipp::char_at( src, 1 ) ) ) )
	{
		char_t buff[char_t_buffer_size] = { };
		buff[0] = ch;

		result.type = token_type::number;
		auto lastChar = ch;
		bool containsDot = false;
		bool containsExponent = false;

		while ( tokenLength < lipp::size( src ) )
		{
			auto ch = src[tokenLength];

			/**/ if ( ch == 'e' && strchr( numDotChars, lastChar ) )
			{
				if ( containsExponent )
				{
					_error = error_type::syntax_error;
					return false;
				}

				containsExponent = true;
			}
			else if ( ( ch == '+' || ch == '-' ) && ( lastChar != '.' && lastChar != 'e' ) )
			{
				_error = error_type::syntax_error;
				return false;
			}
			else if ( ch == '.' && ( strchr( numChars, lastChar ) || lastChar == '+' || lastChar == '-' ) )
			{
				if ( containsDot )
				{
					_error = error_type::syntax_error;
					return false;
				}

				containsDot = true;
			}
			else if ( ch == 'f' )
			{
				if ( !strchr( numChars, lastChar ) )
				{
					_error = error_type::syntax_error;
					return false;
				}

				break;
			}
			else if ( !strchr( numChars, ch ) )
				break;

			if ( tokenLength < char_t_buffer_size - 1 )
				buff[tokenLength] = ch;

			lastChar = ch;
			++tokenLength;
		}
	}
	else if ( strchr( "'\"", ch ) )
	{
		result.type = token_type::string;
		auto lastChar = ch;

		while ( tokenLength < lipp::size( src ) )
		{
			auto c = src[tokenLength++];
			if ( ch == c && lastChar != '\\' )
				break;

			lastChar = c;
		}

		if ( tokenLength < 2 || src[tokenLength - 1] != ch )
		{
			_error = error_type::invalid_string;
			return false;
		}

		result.text = substr( src, 1, tokenLength - 2 );

		if ( flags & parsing_flags::unescape_strings )
		{
			_tempString = string_t();

			for ( size_t i = 0, S = lipp::size( result.text ); i < S; ++i )
			{
				if ( result.text[i] == '\\' )
				{
					// TODO
					++i;
				}
				else
					_tempString += result.text[i];
			}

			result.text = _tempString;
		}

		src = substr( src, tokenLength );
		_cursor += tokenLength;
		return true;
	}
	else if ( strchr( "!@#$%^&*()[]{}<>.,:;+-/*=|?~", ch ) )
	{
		auto secondChar = lipp::char_at( src, 1 );

		/**/ if ( ch == '(' ) result.type = token_type::parent_left;
		else if ( ch == ')' ) result.type = token_type::parent_right;
		else if ( ch == '{' ) result.type = token_type::brace_left;
		else if ( ch == '}' ) result.type = token_type::brace_right;
		else if ( ch == '+' ) result.type = token_type::add;
		else if ( ch == '-' ) result.type = token_type::subtract;
		else if ( ch == '/' ) result.type = token_type::divide;
		else if ( ch == '*' ) result.type = token_type::multiply;
		else if ( ch == ';' ) result.type = token_type::semicolon;
		else if ( ch == '&' && secondChar == '&' )
		{
			result.type = token_type::logical_and;
			++tokenLength;
		}
		else if ( ch == '|' && secondChar == '|' )
		{
			result.type = token_type::logical_or;
			++tokenLength;
		}
		else if ( ch == '=' && secondChar == '=' )
		{
			result.type = token_type::equal;
			++tokenLength;
		}
		else if ( ch == '!' && secondChar == '=' )
		{
			result.type = token_type::not_equal;
			++tokenLength;
		}
		else if ( ch == '<' && secondChar == '=' )
		{
			result.type = token_type::less_equal;
			++tokenLength;
		}
		else if ( ch == '>' && secondChar == '=' )
		{
			result.type = token_type::greater_equal;
			++tokenLength;
		}
		else if ( ch == '!' ) result.type = token_type::logical_not;
		else if ( ch == '<' ) result.type = token_type::less;
		else if ( ch == '>' ) result.type = token_type::greater;
		else if ( ch == '=' ) result.type = token_type::assign;
	}

	result.text = substr( src, 0, tokenLength );
	_cursor += tokenLength;

	if ( result.type == token_type::identifier && !!( flags & parsing_flags::expand_macros ) )
	{
		if ( const auto *value = find_macro( result.text ); value != nullptr )
		{
			auto cursorRewind = _cursor - tokenLength - whitespaceLength;

			_source = substr( _source, cursorRewind, whitespaceLength ) + string_t( value ) + substr( _source, _cursor );
			_cursor = 0;

			result = token();
			return parse_next_token( result, flags );
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename preprocessor<T>::string_t preprocessor<T>::read_all() LIPP_NOEXCEPT
{
	string_t result = "";

	token t;
	while ( next_token( t ) )
	{
		result += t.whitespace;
		result += t.text;
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::concat_remaining_tokens( string_t &result ) LIPP_NOEXCEPT
{
	token t;
	while ( parse_next_token( t, parsing_flags::stop_at_eols | parsing_flags::unescape_strings ) )
	{
		if ( lipp::size( result ) )
			result += " ";

		result += string_t( t.text );
	}

	return _error == error_type::none;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::process_directive( token &result ) LIPP_NOEXCEPT
{
	auto nextIdentifier = [this]()->string_view_t
	{
		if ( token t; parse_next_token( t, 0 ) )
			return t.type == token_type::identifier ? t.text : string_view_t();

		_error = error_type::expected_identifier;
		return string_view_t();
	};

	auto directiveName = nextIdentifier();
	if ( !lipp::size( directiveName ) )
		return false;

	/**/ if ( directiveName == "line" )
	{
		token t;
		if ( !parse_next_token( t, 0 ) || t.type != token_type::number )
		{
			_error = error_type::syntax_error;
			return false;
		}

		_lineNumber = atoi( data( t.text ) ) - 1;

		if ( !parse_next_token( t, parsing_flags::stop_at_eols ) || t.type != token_type::string )
		{
			_error = error_type::syntax_error;
			return false;
		}

		_sourceName = t.text;

		// Resolve current working directory
		{
			size_t slashPos = lipp::size( _sourceName ) - 1;
			while ( slashPos < lipp::size( _sourceName ) && _sourceName[slashPos] != '/' && _sourceName[slashPos] != '\\' )
				--slashPos;

			if ( slashPos < lipp::size( _sourceName ) )
				_cwd = substr( string_view_t( _sourceName ), 0, slashPos );
			else
				_cwd = string_view_t();
		}

		char_t buff[char_t_buffer_size] = { };
		LIPP_SPRINTF( buff, "#line %d \"%s\"", _lineNumber + 1, data( _sourceName ) );

		result.text = buff;
		return true;
	}
	else if ( directiveName == "define" )
	{
		if ( auto macroName = nextIdentifier(); lipp::size( macroName ) )
		{
			string_t value;

			if ( !concat_remaining_tokens( value ) )
				return false;

			define( macroName, value );

			_tempString = "#define ";
			_tempString += macroName;
			_tempString += " ";
			_tempString += value;

			result.text = _tempString;
			return true;
		}

		return false; // Error!
	}
	else if ( directiveName == "undef" )
	{
		if ( auto macroName = nextIdentifier(); lipp::size( macroName ) )
		{
			undef( macroName );

			_tempString = "#undef ";
			_tempString += macroName;

			result.text = _tempString;
			return true;
		}

		return false;
	}
	else if ( directiveName == "ifdef" )
	{
		if ( auto macroName = nextIdentifier(); lipp::size( macroName ) )
		{
			_ifBits = ( _ifBits << 2ull ) | ( ( find_macro( macroName ) != nullptr ) ? 0b11ull : 0b10ull );
			return parse_next_token( result );
		}

		return false;
	}
	else if ( directiveName == "ifndef" )
	{
		if ( auto macroName = nextIdentifier(); lipp::size( macroName ) )
		{
			_ifBits = ( _ifBits << 2ull ) | ( ( find_macro( macroName ) == nullptr ) ? 0b11ull : 0b10ull );
			return parse_next_token( result );
		}

		return false;
	}
	else if ( directiveName == "if" )
	{

	}
	else if ( directiveName == "else" )
	{
		if ( _ifBits )
		{
			_ifBits ^= 1; // Flip first bit
			return parse_next_token( result );
		}

		_error = error_type::mismatch_if;
		return false;
	}
	else if ( directiveName == "endif" )
	{
		if ( _ifBits )
		{
			_ifBits >>= 2;
			return parse_next_token( result );
		}

		_error = error_type::mismatch_if;
		return false;
	}
	else if ( directiveName == "eval" )
	{
		_tempString = result.whitespace;

		auto evalResult = evaluate_expression();
		if ( _error != error_type::none )
			return false;

		result.type = token_type::number;
		result.whitespace = _tempString;

		char_t buff[char_t_buffer_size] = { };
		LIPP_SPRINTF( buff, "%d", evalResult );
		result.text = buff;
		return true;
	}
	else if ( directiveName == "include" )
	{
		token t;
		if ( !parse_next_token( t, parsing_flags::stop_at_eols ) )
		{
			_error = error_type::syntax_error;
			return false;
		}

		bool isSystemPath = t.type == token_type::less;
		string_t fileName;

		if ( t.type == token_type::string )
			fileName = t.text;
		else if ( t.type == token_type::less )
		{
			while ( parse_next_token( t, parsing_flags::stop_at_eols ) )
			{
				if ( t.type == token_type::greater || t.type == token_type::string )
					break;

				fileName += t.whitespace;
				fileName += t.text;
			}

			if ( _error != error_type::none )
				return false;
			else if ( t.type != token_type::greater )
			{
				_error = error_type::invalid_path;
				return false;
			}
		}
		else
		{
			_error = error_type::invalid_path;
			return false;
		}

		// Remember already consumed whitespace from current source
		_tempString = result.whitespace;

		_source = substr( _source, _cursor );
		_cursor = 0;

		if ( !include_file( fileName, isSystemPath ) )
		{
			_error = error_type::include_error;
			return false;
		}

		_source = _tempString + _source;

		return parse_next_token( result );
	}
	else
	{
		return process_unknown_directive( directiveName );
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::evaluate_expression() LIPP_NOEXCEPT
{
	token_type operatorStack[expression_stack_size] = { };
	size_t operatorStackSize = 0;

	int valueStack[expression_stack_size] = { };
	size_t valueStackSize = 0;

	auto popOperator = [&]()->bool
	{
		if ( !operatorStackSize || valueStackSize < 2 )
			return false;

		auto op = operatorStack[--operatorStackSize];
		auto y = valueStack[--valueStackSize];
		auto x = valueStack[--valueStackSize];
		decltype( x ) z = 0;

		/**/ if ( op == token_type::add )
			z = x + y;
		else if ( op == token_type::subtract )
			z = x - y;
		else if ( op == token_type::less )
			z = ( x < y ) ? 1 : 0;
		else if ( op == token_type::less_equal )
			z = ( x <= y ) ? 1 : 0;
		else if ( op == token_type::greater )
			z = ( x > y ) ? 1 : 0;
		else if ( op == token_type::greater_equal )
			z = ( x >= y ) ? 1 : 0;
		else if ( op == token_type::equal )
			z = ( x == y ) ? 1 : 0;
		else if ( op == token_type::not_equal )
			z = ( x != y ) ? 1 : 0;
		else
			return false;

		valueStack[valueStackSize++] = z;
		return true;
	};

	token t;
	while ( parse_next_token( t, parsing_flags::stop_at_eols | parsing_flags::expand_macros ) )
	{
		bool addOperator = false;

		if ( t.type == token_type::number )
		{
			if ( valueStackSize == expression_stack_size )
			{
				set_error( error_type::expression_too_complex );
				return 0;
			}

			valueStack[valueStackSize++] = atoi( data( t.text ) );
		}
		else if ( t.type == token_type::identifier && t.text == "defined" )
		{
			if ( !parse_next_token( t, parsing_flags::stop_at_eols ) || t.type != token_type::parent_left )
			{
				set_error( error_type::syntax_error );
				return 0;
			}

			if ( !parse_next_token( t, parsing_flags::stop_at_eols ) || t.type != token_type::identifier )
			{
				set_error( error_type::expected_identifier );
				return 0;
			}

			if ( valueStackSize == expression_stack_size )
			{
				set_error( error_type::expression_too_complex );
				return 0;
			}

			if ( find_macro( t.text ) )
				valueStack[valueStackSize++] = 1;
			else
				valueStack[valueStackSize++] = 0;

			if ( !parse_next_token( t, parsing_flags::stop_at_eols ) || t.type != token_type::parent_right )
			{
				set_error( error_type::syntax_error );
				return 0;
			}
		}
		else if ( t.type == token_type::add )
			addOperator = true;
		else if ( t.type == token_type::subtract )
			addOperator = true;
		else if ( t.type == token_type::less )
			addOperator = true;
		else if ( t.type == token_type::less_equal )
			addOperator = true;
		else if ( t.type == token_type::greater )
			addOperator = true;
		else if ( t.type == token_type::greater_equal )
			addOperator = true;
		else if ( t.type == token_type::equal )
			addOperator = true;
		else
		{
			set_error( error_type::syntax_error );
			return 0;
		}

		if ( addOperator )
		{
			if ( operatorStackSize == expression_stack_size )
			{
				set_error( error_type::expression_too_complex );
				return 0;
			}

			operatorStack[operatorStackSize++] = t.type;
		}
	}

	while ( operatorStackSize > 0 )
	{
		if ( !popOperator() )
		{
			set_error( error_type::invalid_expression );
			return 0;
		}
	}

	if ( valueStackSize != 1 )
	{
		set_error( error_type::invalid_expression );
		return 0;
	}

	return valueStack[0];
}

} // namespace lipp
