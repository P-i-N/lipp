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
	operator_hash,
	operator_plus,
	operator_minus,
	operator_unknown,
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

	static constexpr char *eol = "\r\n";
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

	static bool read_file( const string_t &fileName, string_t &output ) LIPP_NOEXCEPT
	{
#if !defined(LIPP_DO_NOT_USE_STL)
		std::ifstream ifs( fileName, std::ios_base::in | std::ios_base::binary );
		if ( !ifs.is_open() )
			return false;

		ifs.seekg( 0, std::ios_base::end );
		size_t len = ifs.tellg();
		ifs.seekg( 0, std::ios_base::beg );

		std::unique_ptr<char[]> data( new char[len + 1] );
		ifs.read( data.get(), len );
		data[len] = 0;

		output = data.get();
		return true;
#else
		return false;
#endif
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

	virtual bool define( string_view_t name, string_view_t value );

	bool define( string_view_t name ) { return define( name, "" ); }

	virtual bool undef( string_view_t name );

	virtual const char *find_macro( string_view_t name ) const LIPP_NOEXCEPT;

	virtual void reset() LIPP_NOEXCEPT;

	const string_t &output() const LIPP_NOEXCEPT { return _output; }

	virtual bool include_string( string_view_t src, string_view_t sourceName ) LIPP_NOEXCEPT;

	bool include_string( string_view_t src ) LIPP_NOEXCEPT { return include_string( src, "" ); }

	virtual bool include_file( string_view_t fileName, bool isSystemPath ) LIPP_NOEXCEPT;

	bool include_file( string_view_t fileName ) LIPP_NOEXCEPT { return include_file( fileName, false ); }

	virtual void write_line( string_view_t line ) LIPP_NOEXCEPT;

	virtual void insert_line_directive( int lineNumber, string_view_t fileName ) LIPP_NOEXCEPT;

	void insert_line_directive() LIPP_NOEXCEPT { insert_line_directive( _state.lineNumber, _state.sourceName ); }

protected:
	static string_view_t substr( const string_view_t &sv, size_t offset, size_t length ) LIPP_NOEXCEPT;

	static string_view_t substr( const string_view_t &sv, size_t offset ) LIPP_NOEXCEPT;

	static string_view_t trim( string_view_t str, bool left = true, bool right = true ) LIPP_NOEXCEPT;

	virtual int process_unknown_directive( string_view_t name ) LIPP_NOEXCEPT { return 1; }

	token next_token() LIPP_NOEXCEPT;

	string_view_t next_identifier() LIPP_NOEXCEPT;

	bool process_current_line();

	int process_current_directive();

	int evaluate_expression( string_view_t expr ) const;

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
template <class T> inline bool preprocessor<T>::define( string_view_t name, string_view_t value )
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
template <class T> inline bool preprocessor<T>::undef( string_view_t name )
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
	if ( !traits_t::read_file( buff, fileContent ) )
		return false;

	return include_string( fileContent, buff );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline void preprocessor<T>::write_line( string_view_t line ) LIPP_NOEXCEPT
{
	_output += line;
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

	write_line( string_view_t( buff, T::size( buff ) ) );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline
typename T::string_view_t preprocessor<T>::substr( const string_view_t &sv, size_t offset, size_t length ) LIPP_NOEXCEPT
{
	return string_view_t( T::data( sv ) + offset, length );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline
typename T::string_view_t preprocessor<T>::substr( const string_view_t &sv, size_t offset ) LIPP_NOEXCEPT
{
	return string_view_t( T::data( sv ) + offset, T::size( sv ) - offset );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename T::string_view_t preprocessor<T>::trim( string_view_t str, bool left, bool right ) LIPP_NOEXCEPT
{
	size_t start = 0;
	size_t end = T::size( str );

	if ( left )
	{
		while ( start < end && str[start] <= 32 )
			++start;
	}

	if ( right )
	{
		while ( end > start && str[end - 1] <= 32 )
			--end;
	}

	return substr( str, start, end - start );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename preprocessor<T>::token preprocessor<T>::next_token() LIPP_NOEXCEPT
{
	token result;

	auto &line = _state.currentLine;
	size_t whitespaceLength = 0;

	if ( _state.insideCommentBlock )
	{
		while ( ( T::size( line ) - whitespaceLength ) >= 2 )
		{
			if ( line[whitespaceLength] == '*' && line[whitespaceLength + 1] == '/' )
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

	if ( !T::size( line ) )
		return { token_type::empty };

	// Skip comments
	if ( ( T::size( line ) >= 2 ) && line[0] == '/' )
	{
		/**/ if ( line[1] == '/' )
		{
			line = string_view_t();
			return { token_type::empty };
		}
		else if ( line[1] == '*' )
		{
			_state.insideCommentBlock = true;
			line = substr( line, 2 );
			return next_token();
		}
	}

	{
		static constexpr char *alphaNumChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";

		auto firstChar = _state.currentLine[0];
		size_t length = 1;

		/**/ if ( firstChar == '#' )
		{
			result.type = token_type::operator_hash;
		}
		else if ( strchr( alphaNumChars, firstChar ) )
		{
			result.type = token_type::identifier;
			while ( strchr( alphaNumChars, line[length] ) )
				++length;
		}
		else if ( strchr( "'\"", firstChar ) )
		{
			result.type = token_type::string;
			// TBD...
		}
		else if ( strchr( "!@#$%^&*()[]{}<>.,:;+-/*=|?~", firstChar ) )
		{
			result.type = token_type::operator_unknown;
			// TBD...
		}

		result.text = substr( line, 0, length );
		line = substr( line, length );
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> typename preprocessor<T>::string_view_t preprocessor<T>::next_identifier() LIPP_NOEXCEPT
{
	token t = next_token();
	return t.type == token_type::identifier ? t.text : string_view_t();
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::process_current_line()
{
	bool shouldOutput = true;
	size_t directivePos( -1 );

	auto line = _state.currentLine;

	while ( T::size( _state.currentLine ) )
	{
		auto t = next_token();

		if ( t.type == token_type::operator_hash )
		{
			if ( auto err = process_current_directive(); err < 0 )
				return err;
			else
				shouldOutput = ( err > 0 );
		}
	}

	if ( shouldOutput && !( ( _state.ifBits + 1ull ) & _state.ifBits ) )
		write_line( line );

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::process_current_directive()
{
	auto t = next_token();
	if ( t.type != token_type::identifier )
		return -1;

	auto directive = t.text;

	/**/ if ( directive == "define" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
		{
			define( macroName, trim( _state.currentLine ) );
			_state.currentLine = string_view_t();
			return 1;
		}

		return -1; // Error!
	}
	else if ( directive == "undef" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
		{
			undef( macroName );
			return 1;
		}

		return -1; // Error!
	}
	else if ( directive == "ifdef" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
			_state.ifBits = ( _state.ifBits << 2ull ) | ( ( find_macro( macroName ) != nullptr ) ? 0b11ull : 0b10ull );
		else
			return -1; // Error!
	}
	else if ( directive == "ifndef" )
	{
		if ( auto macroName = next_identifier(); T::size( macroName ) )
			_state.ifBits = ( _state.ifBits << 2ull ) | ( ( find_macro( macroName ) == nullptr ) ? 0b11ull : 0b10ull );
		else
			return -1; // Error!
	}
	else if ( directive == "else" )
	{
		if ( !_state.ifBits )
			return -1;

		_state.ifBits ^= 1; // Flip first bit
	}
	else if ( directive == "endif" )
	{
		if ( !_state.ifBits )
			return -1;

		_state.ifBits >>= 2;
	}
	else if ( directive == "include" )
	{
		auto path = trim( _state.currentLine );
		_state.currentLine = string_view_t();

		if ( T::size( path ) < 3 )
			return -1;

		auto firstCh = path[0];
		auto lastCh = path[T::size( path ) - 1];
		bool isSystemPath = false;

		if ( ( firstCh == '"' && lastCh == '"' ) || ( firstCh == '\'' && lastCh == '\'' ) )
			isSystemPath = false;
		else if ( firstCh == '<' && lastCh == '>' )
			isSystemPath = true;
		else
			return -1;

		auto fileName = substr( path, 1, T::size( path ) - 2 );
		include_file( fileName, isSystemPath );
	}
	else
	{
		return process_unknown_directive( directive );
	}

	return 0; // Do not output
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::evaluate_expression( string_view_t expr ) const
{
	return 0;
}

} // namespace lipp
