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

	string_view_t current_source_name() const LIPP_NOEXCEPT;

	int current_line_number() const LIPP_NOEXCEPT;

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

	struct source_state
	{
		string_t sourceName = string_t();
		string_view_t cwd = string_view_t();

		string_t source = string_t();

		size_t cursor = 0;

		int lineNumber = 1;
		bool toBeRemoved = false;
		bool emitLineDirective = true;
	};

	using states_t = vector_t<source_state>;

	states_t _states;

	auto &current_state() LIPP_NOEXCEPT { return _states[lipp::size( _states ) - 1]; }

	const auto &current_state() const LIPP_NOEXCEPT { return _states[lipp::size( _states ) - 1]; }

	error_type _error = error_type::none;

	unsigned long long _ifBits = 0;

	bool _insideCommentBlock = false;

	string_t _tempString = string_t();
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
	clear( _states );
	_error = error();
	_ifBits = 0;
	_insideCommentBlock = false;
	_tempString = string_t();
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::include_string( string_view_t src, string_view_t sourceName ) LIPP_NOEXCEPT
{
	if ( !lipp::size( src ) )
		return true;

	push_back( _states, {} );
	auto &state = current_state();
	state.source = src;
	state.sourceName = sourceName;

	// Resolve current working directory
	{
		size_t slashPos = lipp::size( sourceName ) - 1;
		while ( slashPos < lipp::size( sourceName ) && sourceName[slashPos] != '/' && sourceName[slashPos] != '\\' )
			--slashPos;

		if ( slashPos < lipp::size( sourceName ) )
			state.cwd = string_view_t( data( state.sourceName ), slashPos );
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::include_file( string_view_t fileName, bool isSystemPath ) LIPP_NOEXCEPT
{
	char_t buff[char_t_buffer_size] = { };

	if ( !isSystemPath && lipp::size( _states ) )
	{
		auto &state = current_state();
		if ( lipp::size( state.cwd ) )
		{
			LIPP_SPRINTF( buff, "%.*s/%.*s",
			              int( lipp::size( state.cwd ) ),
			              data( state.cwd ),
			              int( lipp::size( fileName ) ),
			              data( fileName ) )
		}
	}

	if ( !buff[0] )
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
template <class T> inline
typename T::string_view_t preprocessor<T>::current_source_name() const LIPP_NOEXCEPT
{
	return T::size( _states ) ? current_state().sourceName : string_view_t();
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::current_line_number() const LIPP_NOEXCEPT
{
	return T::size( _states ) ? current_state().lineNumber : 0;
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
	result = token();

	while ( lipp::size( _states ) > 0 && _states[lipp::size( _states ) - 1].toBeRemoved )
	{
		if ( _ifBits )
		{
			_error = error_type::mismatch_if;
			return false;
		}

		pop_back( _states );

		if ( lipp::size( _states ) )
			current_state().emitLineDirective = true;
	}

	if ( lipp::size( _states ) == 0 )
		return false;

	auto &state = current_state();
	auto src = lipp::substr( string_view_t( state.source ), state.cursor );

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

			++state.lineNumber;
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
	state.cursor += whitespaceLength;

	if ( state.emitLineDirective )
	{
		char_t buff[char_t_buffer_size] = { };

		LIPP_SPRINTF( buff, "#line %d \"%s\"\n", state.lineNumber, data( state.sourceName ) );
		_tempString = buff;

		result.type = token_type::directive;
		result.text = _tempString;

		state.emitLineDirective = false;
		return true;
	}

	if ( lipp::size( src ) == 0 )
	{
		if ( _insideCommentBlock )
		{
			_error = error_type::unexpected_eof;
			return false;
		}

		state.toBeRemoved = true;
		result.type = token_type::eof;
		return true;
	}

	static constexpr char_t *alphaChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";
	static constexpr char_t *numChars = "0123456789";
	static constexpr char_t *numDotChars = "0123456789.";

	size_t tokenLength = 1;

	/**/ if ( auto ch = lipp::char_at( src, 0 ); ch == '#' )
	{
		src = substr( src, 1 ); // Cut away '#'
		state.cursor += 1;
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

		if ( flags & parsing_flags::unescape_strings )
		{
			_tempString = string_t();
			result.text = _tempString;
		}
		else
			result.text = substr( src, 1, tokenLength - 2 );

		src = substr( src, tokenLength );
		state.cursor += tokenLength;
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
	state.cursor += tokenLength;

	if ( result.type == token_type::identifier && !!( flags & parsing_flags::expand_macros ) )
	{
		if ( const auto *value = find_macro( result.text ); value != nullptr )
		{
			result.text = value;
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
	auto &state = current_state();

	auto nextIdentifier = [this]( token & result )->string_view_t
	{
		if ( parse_next_token( result, 0 ) )
			return result.type == token_type::identifier ? result.text : string_view_t();

		_error = error_type::expected_identifier;
		return string_view_t();
	};

	auto directiveName = nextIdentifier( result );
	if ( !lipp::size( directiveName ) )
		return false;

	/**/ if ( directiveName == "define" )
	{
		if ( auto macroName = nextIdentifier( result ); lipp::size( macroName ) )
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
		if ( auto macroName = nextIdentifier( result ); lipp::size( macroName ) )
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
		if ( auto macroName = nextIdentifier( result ); lipp::size( macroName ) )
		{
			_ifBits = ( _ifBits << 2ull ) | ( ( find_macro( macroName ) != nullptr ) ? 0b11ull : 0b10ull );
			return parse_next_token( result );
		}

		return false;
	}
	else if ( directiveName == "ifndef" )
	{
		if ( auto macroName = nextIdentifier( result ); lipp::size( macroName ) )
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
			state.emitLineDirective |= is_inside_true_block();
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
			state.emitLineDirective |= is_inside_true_block();
			return parse_next_token( result );
		}

		_error = error_type::mismatch_if;
		return false;
	}
	else if ( directiveName == "print" )
	{
		evaluate_expression();
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

		if ( !include_file( fileName, isSystemPath ) )
		{
			_error = error_type::include_error;
			return false;
		}
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
	token valueStack[expression_stack_size] = { };
	size_t stackSize = 0;

	return 0;
}

} // namespace lipp
