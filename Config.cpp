#include "Config.h"
#include "Common.h"

#define BUFLEN 1024

char padkeyname[][ 3 ] =
{
	"上", "下", "左", "右",
	"Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ", "Ｊ", "Ｋ", "Ｌ",
};

Config::Config( void )
{
	baseurl = NULL;
	gamedir = NULL;
	categorymax = 11;

	newgame = category = dvd = hidetaskbar = fullscreen = 0;
	esccannon[ 0 ] = esccannon[ 1 ] = esccannon[ 2 ] = esccannon[ 3 ] = esccannon[ 4 ] = esccannon[ 5 ] = -1;
	ucur = -1;
	kcesccannon = -1;
	subaddr[ 0 ] = '\0';
}

Config::~Config( void )
{
	free( baseurl );
	free( gamedir );
}

char * Config::CopyString( const char *str )
{
	return CopyString( str, strlen( str ) );
}

char * Config::CopyString( const char *str, unsigned int size )
{
	char *ret;
	ret = (char *)calloc( size + 1, sizeof( str ) );
	strncpy_s( ret, size + 1, str, size );
	ret[ size ] = '\0';
	return ret;
}


int Config::AtoI( const char *str )
{
	return AtoI( str, 0 );
}

int Config::AtoI( const char *str, int figure )
{
	int num = 0, flag = 1;
	int load = 0;

	if(str[ load ] == '-')
	{
		flag = -1;
		load = 1;
	}

	while('0' <= str[ load ] && str[ load ] <= '9')
	{
		num = num * 10 + ( str[ load ] - '0' );
		++load;
		if(--figure == 0)
		{
			break;
		}
	}

	return num * flag;
}

int Config::Load( const char *file )
{
	char buf[ BUFLEN ];
	char *val;
	int i;

	if(MikanFile->Open( 0, file ) >= 0)
	{
		msg.SetMessageFormat( "[ OK ] Load config file : %s", file );

		while(MikanFile->ReadLine( 0, buf, BUFLEN ))
		{
			val = strchr( buf, '\n' );
			if(val)
			{
				*val = '\0';
			}
			val = strchr( buf, '\r' );
			if(val)
			{
				*val = '\0';
			}

			val = strchr( buf, '=' );
			if(val)
			{
				*val = '\0';
				++val;
			}

			if(buf[ 0 ] == '#' || ( buf[ 0 ] == '/' && buf[ 1 ] == '/' ))
			{
				continue;
			} else if(strcmp( buf, "BASEURL" ) == 0)
			{
				if(strncmp( val, "http://", 6 ))
				{
					continue;
				}
				// Ten-LAN Game UploaderのあるURL
				baseurl = CopyString( val );
				if(baseurl[ strlen( baseurl ) - 1 ] == '/')
				{
					baseurl[ strlen( baseurl ) - 1 ] = '\0';
				}
				msg.SetMessageFormat( "BaseURL = %s", baseurl );
			} else if(strcmp( buf, "GAMEDIR" ) == 0)
			{
				// ゲームディレクトリ。
				gamedir = CopyString( val );
				msg.SetMessageFormat( "GameDirectory = %s", gamedir );
			} else if(strcmp( buf, "CATEGORYMAX" ) == 0)
			{
				// カテゴリ数。
				categorymax = AtoI( val );
				msg.SetMessageFormat( "CategoryMax = %d", categorymax );
			} else if(strcmp( buf, "SOUND" ) == 0)
			{
				// 音量。
			} else if(strcmp( buf, "FULLSCREEN" ) == 0)
			{
				// 擬似フルスクリーン。
				if(strncmp( val, "true", 4 ) == 0)
				{
					fullscreen = 1;
				} else
				{
					fullscreen = 0;
				}
			} else if(strcmp( buf, "HIDETASKBAR" ) == 0)
			{
				// タスクバーを隠す。
				if(strncmp( val, "true", 4 ) == 0)
				{
					hidetaskbar = 1;
				} else
				{
					hidetaskbar = 0;
				}
			} else if(strcmp( buf, "VIEWDVD" ) == 0)
			{
				// DVD収録。
				if(strncmp( val, "true", 4 ) == 0)
				{
					dvd = 1;
				} else
				{
					dvd = 0;
				}
			} else if(strcmp( buf, "VIEWNEWGAME" ) == 0)
			{
				// 新作ゲーム。
				if(strncmp( val, "true", 4 ) == 0)
				{
					newgame = 1;
				} else
				{
					newgame = 0;
				}
			} else if(strcmp( buf, "VIEWCATEGORY" ) == 0)
			{
				// カテゴリ表示。
				if(strncmp( val, "true", 4 ) == 0)
				{
					category = 1;
				} else
				{
					category = 0;
				}
			} else if(strcmp( buf, "PAD" ) == 0)
			{
				// パッドキーコンフィグ。
				sscanf_s( val, "%d,%d,%d,%d,%d,%d,%d", pad, pad + 1, pad + 2, pad + 3, pad + 4, pad + 5, pad + 6 );
			} else if(strcmp( buf, "ESCCANNON" ) == 0)
			{
				// ESC砲。
				while(*val != '\0')
				{
					++val;
					if(*val == ',')
					{
						++val;
						break;
					}
				}
				for(i = 0 ; i < 6 ; ++i)
				{
					if('0' <= val[ 0 ] && val[ 0 ] <= '9')
					{
						esccannon[ i ] = Config::AtoI( val );
						while(*val != '\0')
						{
							++val;
							if(*val == ',')
							{
								++val;
								break;
							}
						}
					} else
					{
						break;
					}
				}
			}

		}
		msg.SetMessage( "Close config file." );
		MikanFile->Close( 0 );
	} else
	{
		msg.SetMessageFormat( "[FAIL] Load config file : %s", file );
	}
	return 0;
}

const char * Config::GetGameDirectory( void )
{
	return gamedir;
}

int Config::GetCategoryMax( void )
{
	return categorymax;
}

const char * Config::GetBaseURL( void )
{
	return baseurl;
}

int Config::Draw( void )
{
	int i;
	int basex, basey;
	basex = 0;
	basey = 40;

	MikanDraw->DrawTexture( 0, basex, basey, 0, 480, 640, 340 );

	// アドレス。
	if(MikanInput->GetMouseNum( 0 ) == 1)
	{
		if(MouseOver( basex + 35, basey + 80, 610, 20 ))
		{
			i = baseurl ? strlen( baseurl ) : 0;
			ucur = MikanInput->GetMousePosX();
			if(i * 10 < ucur)
			{
				ucur = i * 10;
			} else
			{
				ucur -= ucur % 10;
			}
		} else
		{
			ucur = -1;
		}
	}
	// カーソル描画。
	if(0 <= ucur)
	{
		MikanDraw->Printf( 2, basex + 35, basey + 80, "%s", subaddr );
		MikanDraw->DrawTextureScaling( 0, basex + 35 + ucur, basey + 80, 690, 60, 5, 5, 10, 20 );
	} else if(baseurl)
	{
		MikanDraw->Printf( 2, basex + 35, basey + 80, "%s", baseurl );
	}


	// 音量。
	//MikanDraw->Printf( 2, basex + 200, basey + 110, "%3d％", sound );

	// 表示設定。
	// 新作ゲーム。
	MikanDraw->DrawTexture( 0, basex + 70, basey + 140, 660, 40 + 20 * newgame, 20, 20 );
	if(MouseOver( basex + 70, basey + 140, 20, 20 ) && MikanInput->GetMouseNum( 0 ) == 1)
	{
		newgame = ( newgame + 1 ) % 2;
	}

	// カテゴリ表示。
	MikanDraw->DrawTexture( 0, basex + 70, basey + 170, 660, 40 + 20 * category, 20, 20 );
	if(MouseOver( basex + 70, basey + 170, 20, 20 ) && MikanInput->GetMouseNum( 0 ) == 1)
	{
		category = ( category + 1 ) % 2;
	}

	// DVD収録。
	MikanDraw->DrawTexture( 0, basex + 70, basey + 200, 660, 40 + 20 * dvd, 20, 20 );
	if(MouseOver( basex + 70, basey + 200, 20, 20 ) && MikanInput->GetMouseNum( 0 ) == 1)
	{
		dvd = ( dvd + 1 ) % 2;
	}

	// フルスクリーン。
	MikanDraw->DrawTexture( 0, basex + 230, basey + 140, 660, 40 + 20 * fullscreen, 20, 20 );
	if(MouseOver( basex + 230, basey + 140, 20, 20 ) && MikanInput->GetMouseNum( 0 ) == 1)
	{
		fullscreen = ( fullscreen + 1 ) % 2;
	}

	// タスクバー。
	MikanDraw->DrawTexture( 0, basex + 230, basey + 170, 660, 40 + 20 * hidetaskbar, 20, 20 );
	if(MouseOver( basex + 230, basey + 170, 20, 20 ) && MikanInput->GetMouseNum( 0 ) == 1)
	{
		hidetaskbar = ( hidetaskbar + 1 ) % 2;
	}

	// ESC砲。
	for(i = 0 ; i < 6 ; ++i)
	{
		if(esccannon[ i ] < 0)
		{
			break;
		}
		MikanDraw->Printf( 2, basex + 163 + i * 30, basey + 230, "%s", padkeyname[ esccannon[ i ] ] );
	}


	// キーコンフィグ。並び順忘れたからなんとかして。
	i = MikanInput->GetPadWhichButton( 0 );
	//kcesccannon
	if(kcesccannon == 0)
	{
		MikanDraw->Printf( 2, basex + 590 - 3, basey + 183, "？" ); // △
		if(0 <= i)
		{
			pad[ 1 ] = i;
			kcesccannon = -1;
		}
	} else
	{
		MikanDraw->Printf( 2, basex + 590 - 3, basey + 183, "%s", padkeyname[ pad[ 1 ] ] ); // △

	}

	MikanDraw->Printf( 2, basex + 617 - 3, basey + 208, "%s", padkeyname[ pad[ 2 ] ] ); // ○
	MikanDraw->Printf( 2, basex + 590 - 3, basey + 233, "%s", padkeyname[ pad[ 3 ] ] ); // ×
	MikanDraw->Printf( 2, basex + 567 - 3, basey + 208, "%s", padkeyname[ pad[ 4 ] ] ); // □
	MikanDraw->Printf( 2, basex + 450 - 3, basey + 160, "%s", padkeyname[ pad[ 5 ] ] ); // L
	MikanDraw->Printf( 2, basex + 590 - 3, basey + 160, "%s", padkeyname[ pad[ 6 ] ] ); // R


	return 0;
}