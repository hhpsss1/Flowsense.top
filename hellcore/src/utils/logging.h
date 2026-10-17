#pragma once


#define LOG( string_ ) g_logging->log( string_, e_log_type::logtype_default );
#define LOG_WARNING( string_ ) g_logging->log( string_, e_log_type::logtype_warning );
#define LOG_ERROR( string_ ) g_logging->log( string_, e_log_type::logtype_error );

enum class e_log_type : unsigned int {
	logtype_default = 0,
	logtype_warning,
	logtype_error
};

class c_logging {
private:
	FILE* m_pCout;
private:
	std::string log_type_to_string( e_log_type nLogType ) {
		switch ( nLogType ) {
		case e_log_type::logtype_default:
			return xx( "LOG" );
		case e_log_type::logtype_warning:
			return xx( "WARNING" );
		case e_log_type::logtype_error:
			return xx( "ERROR" );
		default:
			return "";
		}
	}
public:
	void log( const std::string_view& to_log, e_log_type log_type ) {
		std::cout << this->log_type_to_string( log_type ) << xx( ": " );
		std::cout << to_log << std::endl;
	}

	void log( int int_val, e_log_type log_type ) {
		log( std::to_string( int_val ), log_type );
	}

	void log( float float_val, e_log_type log_type ) {
		log( std::to_string( float_val ), log_type );
	}

	/*void log( vec3_t vec_val, e_log_type log_type ) {
		log( std::string( std::to_string( vec_val.x ) + ", " + std::to_string( vec_val.y ) + ", " + std::to_string( vec_val.z ) ), log_type );
	}*/

	bool init( void ) {
		bool result = AllocConsole( );
		freopen_s( &m_pCout, xx( "CONOUT$" ), xx( "w" ), stdout );
		return result;
	}

	bool init( const char* console_title ) {
		bool result = init( );
		SetConsoleTitleA( console_title );
		return result;
	}
};
inline auto g_logging = std::make_unique<c_logging>( );
