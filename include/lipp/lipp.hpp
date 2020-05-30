#pragma once

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
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace lipp {

struct error
{
	enum
	{
		none,
	};

	int type = none;
	int line = 0;

	operator int() const LIPP_NOEXCEPT { return type; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class token_type
{
	unknown = 0,
	empty,
	number,
	identifier,
	string,
	directive,
	parent_left,
	parent_right,
	add,
	subtract,
	divide,
	multiply,
	logical_and,
	logical_or,
	equal,
	not_equal,
	less_equal,
	greater_equal,
	logical_not,
	less,
	greater,
	symbol,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class String, class StringView, template <class> class Vector>
struct preprocessor_traits
{
	using string_t = String;
	using string_view_t = StringView;
	template <typename T> using vector_t = Vector<T>;

	static constexpr bool keep_defines = true;
	static constexpr bool keep_undefs = true;
	static constexpr int initial_line_number = 1;

	static constexpr char *eol = "\n";
	static constexpr char path_separator = '/';
	static constexpr bool correct_paths = true;

	static constexpr size_t char_buffer_size = 256;

	static size_t size( const char *str ) LIPP_NOEXCEPT { return str ? strlen( str ) : 0; }
	static size_t size( const string_t &s ) LIPP_NOEXCEPT { return s.size(); }
	static size_t size( const string_view_t &sv ) LIPP_NOEXCEPT { return sv.size(); }

	template <typename T>
	static size_t size( const vector_t<T> &v ) LIPP_NOEXCEPT { return v.size(); }

	static const char *data( const string_t &s ) LIPP_NOEXCEPT { return s.data(); }
	static const char *data( const string_view_t &sv ) LIPP_NOEXCEPT { return sv.data(); }

	template <typename T>
	static void clear( vector_t<T> &v ) LIPP_NOEXCEPT { v.clear(); }

	template <typename T>
	static void push_back( vector_t<T> &v, const T &item ) LIPP_NOEXCEPT { v.push_back( item ); }

	template <typename T>
	static void swap_erase_at( vector_t<T> &v, size_t index ) LIPP_NOEXCEPT
	{
		v[index] = std::move( v.back() );
		v.pop_back();
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
class preprocessor
{
public:
	using traits_t = T;
	using string_t = typename traits_t::string_t;
	using string_view_t = typename traits_t::string_view_t;
	template <typename U> using vector_t = typename traits_t::template vector_t<U>;

	struct token
	{
		token_type type = token_type::unknown;
		string_view_t text;
	};

	preprocessor() = default;

	virtual ~preprocessor() = default;

	virtual bool define( string_view_t name, string_view_t value ) LIPP_NOEXCEPT;

	bool define( string_view_t name ) LIPP_NOEXCEPT { return define( name, "" ); }

	virtual bool undef( string_view_t name ) LIPP_NOEXCEPT;

	virtual const char *find_macro( string_view_t name ) const LIPP_NOEXCEPT;

	virtual void reset() LIPP_NOEXCEPT;

	const string_t &output() const LIPP_NOEXCEPT { return _output; }

	virtual bool include_string( string_view_t src, string_view_t sourceName ) LIPP_NOEXCEPT;

	bool include_string( string_view_t src ) LIPP_NOEXCEPT { return include_string( src, "" ); }

	virtual bool include_file( string_view_t fileName, bool isSystemPath ) LIPP_NOEXCEPT;

	bool include_file( string_view_t fileName ) LIPP_NOEXCEPT { return include_file( fileName, false ); }

	virtual void write( string_view_t text ) LIPP_NOEXCEPT;

	virtual void insert_line_directive( int lineNumber, string_view_t fileName ) LIPP_NOEXCEPT;

	void insert_line_directive() LIPP_NOEXCEPT { insert_line_directive( _state.lineNumber, _state.sourceName ); }

protected:
	static string_view_t substr( string_view_t sv, size_t offset, size_t length ) LIPP_NOEXCEPT;

	static string_view_t substr( string_view_t sv, size_t offset ) LIPP_NOEXCEPT;

	static char char_at( string_view_t s, size_t index ) LIPP_NOEXCEPT;

	virtual bool read_file( string_view_t fileName, string_t &output ) LIPP_NOEXCEPT;

	virtual int process_unknown_directive( string_view_t name ) LIPP_NOEXCEPT { return 1; }

	token next_token() LIPP_NOEXCEPT;

	string_view_t next_identifier() LIPP_NOEXCEPT;

	string_t concat_remaining_tokens() LIPP_NOEXCEPT;

	bool process_current_line() LIPP_NOEXCEPT;

	int process_current_directive() LIPP_NOEXCEPT;

	int evaluate_expression( string_view_t expr ) const LIPP_NOEXCEPT;

	string_t _output;

	struct macro
	{
		string_t name;
		string_t value;
	};

	using macros_t = vector_t<macro>;

	macros_t _macros;

	struct state
	{
		string_view_t cwd = string_view_t();
		string_view_t sourceName = string_view_t();
		string_view_t currentLine = string_view_t();
		bool insideCommentBlock = false;
		unsigned long long ifBits = 0;
		int lineNumber = -1;
	} _state;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
class scoped_copy final
{
public:
	scoped_copy( T &value ) noexcept : _value( value ), _copy( value ) { }
	~scoped_copy() noexcept { _value = _copy; }
	T *operator->() noexcept { return &_copy; }

private:
	T &_value;
	T _copy;
};

} // namespace detail

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::define( string_view_t name, string_view_t value ) LIPP_NOEXCEPT
{
	for ( auto &m : _macros )
	{
		if ( m.name == name )
		{
			m.value = value;
			return true;
		}
	}

	T::push_back( _macros, { string_t( name ), string_t( value ) } );
	return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::undef( string_view_t name ) LIPP_NOEXCEPT
{
	for ( size_t i = 0, S = T::size( _macros ); i < S; ++i )
	{
		if ( _macros[i].name == name )
		{
			T::swap_erase_at( _macros, i );
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline const char *preprocessor<T>::find_macro( string_view_t name ) const LIPP_NOEXCEPT
{
	for ( const auto &m : _macros )
		if ( m.name == name )
			return m.value.c_str();

	return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline void preprocessor<T>::reset() LIPP_NOEXCEPT
{
	_output = string_t();
	T::clear( _macros );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::include_string( string_view_t src, string_view_t sourceName ) LIPP_NOEXCEPT
{
	if ( !T::size( src ) )
		return true;

	detail::scoped_copy prevState = _state;

	_state.sourceName = sourceName;
	_state.lineNumber = traits_t::initial_line_number;

	{
		size_t slashPos = T::size( sourceName ) - 1;
		while ( slashPos < T::size( sourceName ) && sourceName[slashPos] != '/' && sourceName[slashPos] != '\\' )
			--slashPos;

		if ( slashPos < T::size( sourceName ) )
			_state.cwd = string_view_t( T::data( sourceName ), slashPos );
	}

	insert_line_directive();

	while ( T::size( src ) )
	{
		int consumedLines = 1;

		_state.currentLine = string_view_t();

		// Consume line(s) from 'src'
		{
			size_t length = 0;

			char lastLineChar = 0;

			while ( length < T::size( src ) )
			{
				auto ch = src[length++];
				if ( ch == '\n' )
				{
					if ( lastLineChar != '\\' )
						break;

					lastLineChar = 0;
					++consumedLines;
				}
				else if ( ch > 32 )
					lastLineChar = ch;
			}

			_state.currentLine = string_view_t( T::data( src ), length );
			src = substr( src, length );
		}

		process_current_line();
		_state.lineNumber += consumedLines;
	}

	if ( prevState->lineNumber >= traits_t::initial_line_number && _state.sourceName != prevState->sourceName )
		insert_line_directive( prevState->lineNumber, prevState->sourceName );

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::include_file( string_view_t fileName, bool isSystemPath ) LIPP_NOEXCEPT
{
	char buff[T::char_buffer_size] = { };

	if ( !isSystemPath && T::size( _state.cwd ) )
	{
		LIPP_SPRINTF( buff, "%.*s%c%.*s",
		              int( T::size( _state.cwd ) ),
		              T::data( _state.cwd ),
		              T::path_separator,
		              int( T::size( fileName ) ),
		              T::data( fileName ) )
	}
	else
	{
		LIPP_SPRINTF( buff, "%.*s", int( T::size( fileName ) ), T::data( fileName ) )
	}

	if ( traits_t::correct_paths )
	{
		const char wrongSeparator = ( traits_t::path_separator == '/' ) ? '\\' : '/';

		for ( size_t i = 0, S = T::size( buff ); i < S; ++i )
		{
			if ( buff[i] == wrongSeparator )
				buff[i] = traits_t::path_separator;
		}
	}

	string_t fileContent;
	if ( !read_file( buff, fileContent ) )
		return false;

	return include_string( fileContent, buff );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline void preprocessor<T>::write( string_view_t text ) LIPP_NOEXCEPT
{
	_output += text;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline void preprocessor<T>::insert_line_directive( int lineNumber, string_view_t fileName ) LIPP_NOEXCEPT
{
	char buff[T::char_buffer_size] = { };
	LIPP_SPRINTF( buff, "#line %d \"%.*s\"%s",
	              lineNumber,
	              int( T::size( fileName ) ),
	              T::data( fileName ),
	              T::eol )

	write( string_view_t( buff, T::size( buff ) ) );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::read_file( string_view_t fileName, string_t &output ) LIPP_NOEXCEPT
{
#if !defined(LIPP_DO_NOT_USE_STL)
	if ( std::ifstream ifs( string_t( fileName ).c_str() ); ifs.is_open() )
	{
		output = string_t( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );
		return true;
	}
#endif

	return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline
typename T::string_view_t preprocessor<T>::substr( string_view_t sv, size_t offset, size_t length ) LIPP_NOEXCEPT
{
	return string_view_t( T::data( sv ) + offset, length );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline
typename T::string_view_t preprocessor<T>::substr( string_view_t sv, size_t offset ) LIPP_NOEXCEPT
{
	return string_view_t( T::data( sv ) + offset, T::size( sv ) - offset );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline
char preprocessor<T>::char_at( string_view_t s, size_t index ) LIPP_NOEXCEPT
{
	return index < T::size( s ) ? s[index] : 0;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename preprocessor<T>::token preprocessor<T>::next_token() LIPP_NOEXCEPT
{
	token result;

	auto &line = _state.currentLine;
	size_t whitespaceLength = 0;

	if ( _state.insideCommentBlock )
	{
		while ( whitespaceLength < T::size( line ) )
		{
			if ( char_at( line, whitespaceLength ) == '*' && char_at( line, whitespaceLength + 1 ) == '/' )
			{
				_state.insideCommentBlock = false;
				whitespaceLength += 2;
				break;
			}

			++whitespaceLength;
		}

		if ( _state.insideCommentBlock )
		{
			line = string_view_t();
			return { token_type::empty };
		}
	}

	// Skip whitespace
	{
		size_t length = 0;
		while ( whitespaceLength < T::size( line ) && line[whitespaceLength] <= 32 )
			++whitespaceLength;
	}

	line = substr( line, whitespaceLength );

	auto firstChar = char_at( line, 0 );
	if ( !firstChar )
		return { token_type::empty };

	// Skip comments
	if ( firstChar == '/' )
	{
		if ( auto nextCh = char_at( line, 1 ); nextCh == '/' )
		{
			line = string_view_t();
			return { token_type::empty };
		}
		else if ( nextCh == '*' )
		{
			_state.insideCommentBlock = true;
			line = substr( line, 2 );
			return next_token();
		}
	}

	static constexpr char *alphaNumChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$";
	size_t tokenLength = 1;

	/**/ if ( firstChar == '#' )
	{
		result.type = token_type::directive;
	}
	else if ( strchr( alphaNumChars, firstChar ) )
	{
		result.type = token_type::identifier;
		while ( strchr( alphaNumChars, line[tokenLength] ) )
			++tokenLength;
	}
	else if ( strchr( "'\"", firstChar ) )
	{
		result.type = token_type::string;
		auto lastChar = firstChar;

		while ( tokenLength < T::size( line ) )
		{
			auto ch = line[tokenLength++];

			if ( ch == firstChar && lastChar != '\\' )
				break;

			lastChar = ch;
		}

		if ( tokenLength < 2 || line[tokenLength - 1] != firstChar )
			return { token_type::empty };
	}
	else if ( strchr( "!@#$%^&*()[]{}<>.,:;+-/*=|?~", firstChar ) )
	{
		result.type = token_type::symbol;
		auto secondChar = char_at( line, 1 );

		/**/ if ( firstChar == '(' ) result.type = token_type::parent_left;
		else if ( firstChar == ')' ) result.type = token_type::parent_right;
		else if ( firstChar == '+' ) result.type = token_type::add;
		else if ( firstChar == '-' ) result.type = token_type::subtract;
		else if ( firstChar == '/' ) result.type = token_type::divide;
		else if ( firstChar == '*' ) result.type = token_type::multiply;
		else if ( firstChar == '&' && secondChar == '&' )
		{
			result.type = token_type::logical_and;
			++tokenLength;
		}
		else if ( firstChar == '|' && secondChar == '|' )
		{
			result.type = token_type::logical_or;
			++tokenLength;
		}
		else if ( firstChar == '=' && secondChar == '=' )
		{
			result.type = token_type::equal;
			++tokenLength;
		}
		else if ( firstChar == '!' && secondChar == '=' )
		{
			result.type = token_type::not_equal;
			++tokenLength;
		}
		else if ( firstChar == '<' && secondChar == '=' )
		{
			result.type = token_type::less_equal;
			++tokenLength;
		}
		else if ( firstChar == '>' && secondChar == '=' )
		{
			result.type = token_type::greater_equal;
			++tokenLength;
		}
		else if ( firstChar == '!' ) result.type = token_type::logical_not;
		else if ( firstChar == '<' ) result.type = token_type::less;
		else if ( firstChar == '>' ) result.type = token_type::greater;
	}

	result.text = substr( line, 0, tokenLength );
	line = substr( line, tokenLength );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename preprocessor<T>::string_view_t preprocessor<T>::next_identifier() LIPP_NOEXCEPT
{
	auto t = next_token();
	return t.type == token_type::identifier ? t.text : string_view_t();
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename preprocessor<T>::string_t preprocessor<T>::concat_remaining_tokens() LIPP_NOEXCEPT
{
	string_t result = string_t();

	auto &line = _state.currentLine;
	while ( T::size( line ) )
	{
		if ( auto t = next_token(); T::size( t.text ) )
		{
			if ( T::size( result ) )
				result += " ";

			result += string_t( t.text );
		}
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::process_current_line() LIPP_NOEXCEPT
{
	bool shouldOutput = true;

	auto originalLine = _state.currentLine;

	while ( T::size( _state.currentLine ) )
	{
		auto t = next_token();

		if ( t.type == token_type::directive )
		{
			if ( auto err = process_current_directive(); err < 0 )
				return err;
			else
				shouldOutput = ( err > 0 );

			break; // Nothing else to process after directive
		}
	}

	if ( shouldOutput && !( ( _state.ifBits + 1ull ) & _state.ifBits ) )
	{
		write( originalLine );

		if ( auto len = T::size( originalLine ); len && originalLine[len - 1] != '\n' )
			write( traits_t::eol );
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::process_current_directive() LIPP_NOEXCEPT
{
	auto directiveName = next_identifier();
	if ( !T::size( directiveName ) )
		return -1;

	auto &line = _state.currentLine;

	/**/ if ( directiveName == "define" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
		{
			define( macroName, concat_remaining_tokens() );
			_state.currentLine = string_view_t();
			return 1;
		}

		return -1; // Error!
	}
	else if ( directiveName == "undef" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
		{
			undef( macroName );
			return 1;
		}

		return -1; // Error!
	}
	else if ( directiveName == "ifdef" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
			_state.ifBits = ( _state.ifBits << 2ull ) | ( ( find_macro( macroName ) != nullptr ) ? 0b11ull : 0b10ull );
		else
			return -1; // Error!
	}
	else if ( directiveName == "ifndef" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
			_state.ifBits = ( _state.ifBits << 2ull ) | ( ( find_macro( macroName ) == nullptr ) ? 0b11ull : 0b10ull );
		else
			return -1; // Error!
	}
	else if ( directiveName == "if" )
	{

	}
	else if ( directiveName == "else" )
	{
		if ( !_state.ifBits )
			return -1;

		_state.ifBits ^= 1; // Flip first bit
	}
	else if ( directiveName == "endif" )
	{
		if ( !_state.ifBits )
			return -1;

		_state.ifBits >>= 2;
	}
	else if ( directiveName == "print" )
	{

	}
	else if ( directiveName == "include" )
	{
		auto pathToken = next_token();
		bool isSystemPath = pathToken.type == token_type::less;

		/**/ if ( pathToken.type == token_type::string )
		{
			// Trim string quotes
			pathToken.text = substr( pathToken.text, 1, T::size( pathToken.text ) - 2 );
		}
		else if ( pathToken.type == token_type::less )
		{
			size_t length = 0;
			while ( length < T::size( line ) && line[length] != '>' )
				++length;

			if ( length >= T::size( line ) )
				return -1;

			pathToken.text = substr( line, 0, length );
			line = substr( line, length + 1 );
		}
		else
			return -1;

		include_file( pathToken.text, isSystemPath );
	}
	else
	{
		return process_unknown_directive( directiveName );
	}

	return 0; // Do not output
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::evaluate_expression( string_view_t expr ) const LIPP_NOEXCEPT
{


	return 0;
}

} // namespace lipp
