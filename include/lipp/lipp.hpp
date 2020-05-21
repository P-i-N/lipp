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
	int column = 0;

	operator int() const LIPP_NOEXCEPT { return type; }
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

	preprocessor() = default;

	virtual ~preprocessor() = default;

	virtual bool define( string_view_t name, string_view_t value );

	bool define( string_view_t name ) { return define( name, "" ); }

	virtual bool undef( string_view_t name );

	virtual void undef_all();

	virtual const char *find_macro( string_view_t name ) const LIPP_NOEXCEPT;

	virtual bool include_string( string_view_t src, string_view_t sourceName ) LIPP_NOEXCEPT;

	bool include_string( string_view_t src ) LIPP_NOEXCEPT { return include_string( src, "" ); }

	virtual bool include_file( string_view_t fileName, bool isSystemPath ) LIPP_NOEXCEPT;

	bool include_file( string_view_t fileName ) LIPP_NOEXCEPT { return include_file( fileName, false ); }

	virtual void write_line( string_view_t line ) LIPP_NOEXCEPT;

	virtual void write_line_directive( int lineNumber, string_view_t fileName ) LIPP_NOEXCEPT;

	void write_line_directive() LIPP_NOEXCEPT { write_line_directive( _state.lineNumber, _state.sourceName ); }

protected:
	static string_view_t trim( string_view_t str, bool left = true, bool right = true ) LIPP_NOEXCEPT;

	static string_view_t split_token( string_view_t &str ) LIPP_NOEXCEPT;

	static string_view_t split_line( string_view_t &str ) LIPP_NOEXCEPT;

	bool process_line( string_view_t line );

	int process_directive( string_view_t dir );

	int evaluate_expression( string_view_t expr ) const;

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
template <class T> inline void preprocessor<T>::undef_all()
{
	T::clear( _macros );
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

	write_line_directive();

	while ( T::size( src ) )
	{
		auto line = split_line( src );
		process_line( line );
		++_state.lineNumber;
	}

	if ( prevState->lineNumber >= traits_t::initial_line_number && _state.sourceName != prevState->sourceName )
		write_line_directive( prevState->lineNumber, prevState->sourceName );

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
#if !defined(LIPP_DO_NOT_USE_STL)
	printf( "%.*s%s", int( T::size( line ) ), T::data( line ), T::eol );
#endif
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline void preprocessor<T>::write_line_directive( int lineNumber, string_view_t fileName ) LIPP_NOEXCEPT
{
	char buff[T::char_buffer_size] = { };
	LIPP_SPRINTF( buff, "#line %d \"%.*s\"",
	              lineNumber,
	              int( T::size( fileName ) ),
	              T::data( fileName ) )

	write_line( string_view_t( buff, T::size( buff ) ) );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename T::string_view_t preprocessor<T>::trim( string_view_t str, bool left, bool right ) LIPP_NOEXCEPT
{
	size_t start = 0;
	size_t end = T::size( str );

	if ( left )
	{
		while ( start < end && isspace( str[start] ) )
			++start;
	}

	if ( right )
	{
		while ( end > start && isspace( str[end - 1] ) )
			--end;
	}

	return string_view_t( T::data( str ) + start, end - start );
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename T::string_view_t preprocessor<T>::split_token( string_view_t &str ) LIPP_NOEXCEPT
{
	str = trim( str, true, false );

	if ( !T::size( str ) )
		return traits_t::string_view_t();

	static constexpr char *alphaNumChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._";

	char firstChar = *T::data( str );
	size_t length = 0;

	if ( strchr( alphaNumChars, firstChar ) )
	{
		while ( strchr( alphaNumChars, str[length] ) )
			++length;
	}
	else if ( strchr( "'\"", firstChar ) )
	{

	}
	else if ( strchr( "=()&|!><+-/*", firstChar ) )
	{

	}

	auto result = string_view_t( T::data( str ), length );
	str = string_view_t( T::data( str ) + length, T::size( str ) - length );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline typename T::string_view_t preprocessor<T>::split_line( string_view_t &str ) LIPP_NOEXCEPT
{
	if ( !T::size( str ) )
		return string_view_t();

	size_t length = 0;

	while ( length < T::size( str ) && str[length] != '\n' )
		++length;

	auto result = string_view_t( T::data( str ), length );

	if ( length < T::size( str ) )
		++length;

	str = string_view_t( T::data( str ) + length, T::size( str ) - length );
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline bool preprocessor<T>::process_line( string_view_t line )
{
	bool shouldOutput = true;
	size_t directivePos( -1 );

	char ch = 0, lastCh = 0;
	for ( size_t i = 0; i < T::size( line ); ++i, lastCh = ch )
	{
		ch = line[i];
		if ( isspace( ch ) )
			continue;

		if ( _state.insideCommentBlock )
		{
			// End of comment block?
			if ( ch == '/' && lastCh == '*' )
				_state.insideCommentBlock = false;
			else
				continue;
		}

		// Start of comment block
		/**/ if ( ch == '*' && lastCh == '/' )
		{
			_state.insideCommentBlock = true;
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

	if ( directivePos < T::size( line ) )
	{
		++directivePos;

		if ( auto err = process_directive( string_view_t( T::data( line ) + directivePos, T::size( line ) - directivePos ) ); err < 0 )
			return err;
		else
			shouldOutput = ( err > 0 );
	}

	if ( shouldOutput && !( ( _state.ifBits + 1ull ) & _state.ifBits ) )
		write_line( line );

	return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::process_directive( string_view_t dir )
{
	auto dirName = split_token( dir );

	/**/ if ( dirName == "define" )
	{
		if ( auto macroName = split_token( dir ); T::size( macroName ) )
		{
			define( macroName, split_token( dir ) );
			return 1;
		}

		return -1; // Error!
	}
	else if ( dirName == "undef" )
	{
		if ( auto macroName = split_token( dir ); T::size( macroName ) )
		{
			undef( macroName );
			return 1;
		}

		return -1; // Error!
	}
	else if ( dirName == "ifdef" )
	{
		if ( auto macroName = split_token( dir ); T::size( macroName ) )
			_state.ifBits = ( _state.ifBits << 2ull ) | ( ( find_macro( macroName ) != nullptr ) ? 0b11ull : 0b10ull );
		else
			return -1; // Error!
	}
	else if ( dirName == "ifndef" )
	{
		if ( auto macroName = split_token( dir ); T::size( macroName ) )
			_state.ifBits = ( _state.ifBits << 2ull ) | ( ( find_macro( macroName ) == nullptr ) ? 0b11ull : 0b10ull );
		else
			return -1; // Error!
	}
	else if ( dirName == "else" )
	{
		if ( !_state.ifBits )
			return -1;

		_state.ifBits ^= 1; // Flip first bit
	}
	else if ( dirName == "endif" )
	{
		if ( !_state.ifBits )
			return -1;

		_state.ifBits >>= 2;
	}
	else if ( dirName == "include" )
	{
		if ( dir = trim( dir ); T::size( dir ) < 3 )
			return -1;

		auto firstCh = dir[0];
		auto lastCh = dir[T::size( dir ) - 1];
		bool isSystemPath = false;

		if ( ( firstCh == '"' && lastCh == '"' ) || ( firstCh == '\'' && lastCh == '\'' ) )
			isSystemPath = false;
		else if ( firstCh == '<' && lastCh == '>' )
			isSystemPath = true;
		else
			return -1;

		auto fileName = string_view_t( T::data( dir ) + 1, T::size( dir ) - 2 );
		include_file( fileName, isSystemPath );
	}

	return 0; // Do not output
}

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline int preprocessor<T>::evaluate_expression( string_view_t expr ) const
{
	return 0;
}

} // namespace lipp
