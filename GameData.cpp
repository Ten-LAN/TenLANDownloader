#include "GameData.h"
#include "Config.h"

#define BUFLEN 1024

GameData::GameData( void )
{
	gamelist = NULL;
	gamemax = 0;
	x = y = w = h = 0;
	fontsize = 25;
	begin = vgamenum = 0;
	select = -1;
	target = -1;
	mode = 0;
	max = progress = 1;
}

GameData::~GameData( void )
{
	Release();
}

int GameData::Release( void )
{
	struct GameDataList *del;

	while(gamelist)
	{
		del = gamelist;
		gamelist = gamelist->next;
		free( del->title[ GAMEDATA_DIR ] );
		free( del->title[ GAMEDATA_JSON ] );
		free( del->idname );
		free( del->category );
		free( del );
	}

	free( gametable );
	gametable = NULL;
	return 0;
}

int GameData::Init( int categorymax )
{
	this->categorymax = categorymax;

	MikanSystem->CreateLock( 4 );

	MikanDraw->CreateFont( 4, "MigMix 1M", fontsize, 0xFFFFFFFF );

	return 0;
}

unsigned int GameData::GetGameMax( void )
{
	return gamemax;
}

int GameData::IsChecked( unsigned int gamenum )
{
	if(GetGameMax() <= gamenum)
	{
		return 0;
	}
	return gametable[ gamenum ]->check;
}

int GameData::Unckeck( unsigned int gamenum )
{
	if(GetGameMax() <= gamenum)
	{
		return 0;
	}
	gametable[ gamenum ]->check = 0;
	return 0;
}

char * GameData::GetIdName( unsigned int gamenum )
{
	if(GetGameMax() <= gamenum)
	{
		return NULL;
	}
	return gametable[ gamenum ]->idname;
}

int GameData::GetGameNumber( unsigned int gamenum )
{
	if(GetGameMax() <= gamenum)
	{
		return 0;
	}
	return gametable[ gamenum ]->number;
}

int GameData::GetGameVersion( unsigned int gamenum )
{
	if(GetGameMax() <= gamenum)
	{
		return 0;
	}
	return gametable[ gamenum ]->gver[ GAMEDATA_JSON ];
}

int GameData::SetTarget( int target )
{
	if(target < 0 || gamemax <= target)
	{
		this->target = -1;
		return 1;
	}
	this->target = target;
	return 0;
}

int GameData::SetProgressMode( int mode )
{
	if(mode)
	{
		// ダウンロード。
		this->mode = 1;
		return 0;
	}
	this->mode = 0;
	return 0;
}

int GameData::SetProgressMax( int max )
{
	this->max = max;
	return 0;
}

int * GameData::GetProgressMax( void )
{
	return &max;
}

int * GameData::GetProgressNow( void )
{
	return &progress;
}

int GameData::LoadFromDirectory( const char *gamedir )
{
	HANDLE hdir;
	WIN32_FIND_DATA status;
	struct stat fstat;

	char filepath[ 1024 ] = "";
	int n = 0, msel = 0;
	int _loadgame = 0;

	sprintf_s( filepath, 1024, "%s\\*", gamedir );

	if(( hdir = FindFirstFile( filepath, &status ) ) != INVALID_HANDLE_VALUE)
	{
		msg.SetMessageFormat( "[ OK ] Directory open : %s", gamedir );
		do
		{
			if(( status.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ))
			{
				//ディレクトリ
				if(strcmp( status.cFileName, "." ) != 0 &&
					strcmp( status.cFileName, ".." ) != 0)
				{
					sprintf_s( filepath, 1024, "%s\\%s\\%s", gamedir, status.cFileName, "data.ini" );
					stat( filepath, &fstat );
					if(MikanFile->Open( 0, filepath, "r" ) >= 0)
					{
						AnalysisFile( Config::AtoI( status.cFileName ), filepath );
					}
				}
			}
		} while(FindNextFile( hdir, &status ));

		msg.SetMessageFormat( "GameDirectory loaded." );
		FindClose( hdir );
	} else
	{
		msg.SetMessageFormat( "[FAIL] Directory cannot open : %s", gamedir );
	}

	CreateGameTable();

	return 0;
}

// ファイルの解析。
struct GameDataList * GameData::AnalysisFile( int number, const char *file )
{
	struct GameDataList *data, *last;
	char buf[ BUFLEN ];
	char *val;
	int newdata = 0;

	last = NULL;
	data = gamelist;
	while(data)
	{
		if(data->number == number)
		{
			break;
		}
		last = data;
		data = data->next;
	}

	// 新規作成。
	if(data == NULL)
	{
		data = ( struct GameDataList * )calloc( 1, sizeof( struct GameDataList ) );
		if(data)
		{
			data->ver[ GAMEDATA_JSON ] = data->gver[ GAMEDATA_JSON ] = -1;
			data->category = (char *)calloc( categorymax, sizeof( char ) );
			if(data->category)
			{
				// とりあえず最低限必要な領域確保には成功。
				++gamemax;
				if(last)
				{
					// 新規作成かつlastがあるので、末尾に追加。
					last->next = data;
				} else
				{
					// 新規作成かつlastがないので、初めの要素として登録。
					gamelist = data;
				}
			} else
			{
				// 失敗時の処理。
			}
		}
	}

	if(data == NULL || data->category == NULL)
	{
		msg.SetMessageFormat( "[FAIL] Failure allocation new GameDataList." );
		return NULL;
	}

	data->number = number;

	if(MikanFile->Open( 0, file ) >= 0)
	{
		msg.SetMessageFormat( "[ OK ] File open : %s", file );

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

			if(strcmp( buf, "title" ) == 0)
			{
				// ゲームタイトル。
				if(data->title[ GAMEDATA_DIR ])
				{
					free( data->title[ GAMEDATA_DIR ] );
				}
				data->title[ GAMEDATA_DIR ] = Config::CopyString( val );
				msg.SetMessageFormat( "Title = %s", data->title[ GAMEDATA_DIR ] );
			} else if(strcmp( buf, "idname" ) == 0)
			{
				// 固有識別名。
				if(data->idname == NULL && strlen( val ) > 0)
				{
					data->idname = Config::CopyString( val );
				}
				msg.SetMessageFormat( "IDName = %s", data->idname );
			} else if(strcmp( buf, "date" ) == 0)
			{
				// 日付。
				sprintf_s( data->date[ GAMEDATA_DIR ], 30, "%4d/%02d/%02d %02d:%02d:%02d",
					Config::AtoI( val, 4 ), Config::AtoI( val + 4, 2 ), Config::AtoI( val + 6, 2 ),
					Config::AtoI( val + 8, 2 ), Config::AtoI( val + 10, 2 ), Config::AtoI( val + 12, 2 ) );
			} else if(strcmp( buf, "cate" ) == 0)
			{
				// メインカテゴリ。
				data->maincategory[ GAMEDATA_DIR ] = Config::AtoI( val );
				msg.SetMessageFormat( "Category = %s", val );

				// サブカテゴリ。
				data->category[ data->maincategory[ GAMEDATA_DIR ] ] |= 1 << GAMEDATA_DIR;
				val = strchr( val + 1, ',' );
				while(val)
				{
					++val;
					data->category[ Config::AtoI( val ) ] |= 1 << GAMEDATA_DIR;
					val = strchr( val, ',' );
				}
			} else if(strcmp( buf, "ver" ) == 0)
			{
				// 設定のバージョン。
				data->ver[ GAMEDATA_DIR ] = Config::AtoI( val );
				msg.SetMessageFormat( "Version = %d", data->ver[ GAMEDATA_DIR ] );
			} else if(strcmp( buf, "gver" ) == 0)
			{
				// 設定のバージョン。
				data->gver[ GAMEDATA_DIR ] = Config::AtoI( val );
				msg.SetMessageFormat( "GameVersion = %d", data->gver[ GAMEDATA_DIR ] );
			}

		}

		msg.SetMessageFormat( "GameFile loaded." );
		MikanFile->Close( 0 );
	} else
	{
		msg.SetMessageFormat( "[FAIL] File cannot open : %s", file );
	}

	return data;
}

#define JSONBUFLEN 2048

int GameData::GetGameMaxFromJSON( int filenum )
{
	char buf[ 16 ];
	int i;

	do
	{
		MikanFile->Read( filenum, buf, 1 );
	} while(buf[ 0 ] != '"');
	i = 0;
	do
	{
		MikanFile->Read( filenum, buf + i, 1 );
	} while(buf[ i++ ] != '"');
	if(i <= 0)
	{
		i = 1;
	}
	buf[ i - 1 ] = '\0';

	return Config::AtoI( buf );
}

int IsShiftJIS( unsigned char chara )
{
	return ( chara > 0x80 ) && ( ( chara < 0xa0 ) || ( chara>0xdf ) );
}

int GameData::LoadFromJson( const char *jsonfile )
{
	int max = 0;
	char buf[ JSONBUFLEN ];
	int i;

	if(MikanFile->Open( 1, jsonfile ) >= 0)
	{
		do
		{
			MikanFile->Read( 1, buf, 1 );
		} while(buf[ 0 ] != '{');

		while(1)
		{
			buf[ 0 ] = 'a';
			do
			{
				if(MikanFile->Read( 1, buf, 1 ) == 0)
				{
					break;
				}
			} while(buf[ 0 ] != '"');
			if(buf[ 0 ] != '"')
			{
				break;
			}
			i = 1;
			do
			{
				MikanFile->Read( 1, buf + i, 1 );
			} while(buf[ i++ ] != ':');
			buf[ i - 1 ] = '\0';


			if(strcmp( buf, "\"max\"" ) == 0)
			{
				// 最大数取得。
				max = GetGameMaxFromJSON( 1 );
			} else if(strcmp( buf, "\"gamelist\"" ) == 0)
			{
				// ゲームリストへの追加。
				do
				{
					// { を探す。
					do
					{
						MikanFile->Read( 1, buf, 1 );
					} while(buf[ 0 ] != '{');
					i = 1;
					// }までを文字列に入れる。
					do
					{
						MikanFile->Read( 1, buf + i, 1 );
						if(IsShiftJIS( buf[ i ] ))
						{
							++i;
							MikanFile->Read( 1, buf + i, 1 );
							++i;
							MikanFile->Read( 1, buf + i, 1 );
						}
					} while(buf[ i++ ] != '}');
					buf[ i ] = '\0';

					// ここで解析
					AnalysisJson( buf );

					MikanFile->Read( 1, buf, 1 );
				} while(buf[ 0 ] == ',');

			}
		}

		MikanFile->Close( 1 );
	}

	CreateGameTable();
	return 0;
}

struct GameDataList * GameData::AnalysisJson( const char *json )
{
	int number, i;

	struct GameDataList *data, *last = NULL;
	const char *val, *tmp;
	int newdata = 0;

	val = strstr( json, "\"number\":\"" );
	if(val == NULL)
	{
		msg.SetMessageFormat( "[FAIL] Failure analysis json." );
	}
	number = Config::AtoI( val + 10 );

	data = gamelist;
	while(data)
	{
		if(data->number == number)
		{
			break;
		}
		last = data;
		data = data->next;
	}

	if(data == NULL)
	{
		data = ( struct GameDataList * )calloc( 1, sizeof( struct GameDataList ) );

		if(data)
		{
			data->ver[ GAMEDATA_DIR ] = data->gver[ GAMEDATA_DIR ] = -1;
			data->category = (char *)calloc( categorymax, sizeof( char ) );
			if(data->category)
			{
				// とりあえず最低限必要な領域確保には成功。
				++gamemax;
				if(last)
				{
					// 新規作成かつlastがあるので、末尾に追加。
					last->next = data;
				} else
				{
					// 新規作成かつlastがないので、初めの要素として登録。
					gamelist = data;
				}
			} else
			{
				// 失敗時の処理。
			}
		}

	}


	if(data == NULL || data->category == NULL)
	{
		msg.SetMessageFormat( "[FAIL] Failure allocation new GameDataList." );
		return NULL;
	}

	data->number = number;

	i = 0;
	while(1)
	{
		val = strchr( json + i, ':' );
		if(val == NULL)
		{
			break;
		}
		tmp = strchr( json + i, '"' ) + 1;
		i += strchr( val + 2, '"' ) - ( json + i ) + 1;
		val += 2;

		if(strncmp( tmp, "title", 5 ) == 0)
		{
			// ゲームタイトル。
			data->title[ GAMEDATA_JSON ] = Config::CopyString( val, strchr( val + 1, '"' ) - val );
			msg.SetMessageFormat( "Title = %s", data->title[ GAMEDATA_JSON ] );
		} else if(strncmp( tmp, "idname", 6 ) == 0)
		{
			// 固有識別名。
			if(data->idname == NULL && 1 < strlen( val ) && val[ 0 ] != '"')
			{
				data->idname = Config::CopyString( val, strchr( val + 1, '"' ) - val );
			}
			msg.SetMessageFormat( "IDName = %s", data->idname );
		} else if(strncmp( tmp, "date", 4 ) == 0)
		{
			// 日付。
			sprintf_s( data->date[ GAMEDATA_JSON ], 20, "%4d/%02d/%02d %02d:%02d:%02d",
				Config::AtoI( val, 4 ), Config::AtoI( val + 4, 2 ), Config::AtoI( val + 6, 2 ),
				Config::AtoI( val + 8, 2 ), Config::AtoI( val + 10, 2 ), Config::AtoI( val + 12, 2 ) );
		} else if(strncmp( tmp, "cate", 4 ) == 0)
		{
			// メインカテゴリ。
			data->maincategory[ GAMEDATA_JSON ] = Config::AtoI( val );
			msg.SetMessageFormat( "Category = %s", val );

			// サブカテゴリ。
			data->category[ data->maincategory[ GAMEDATA_JSON ] ] |= 1 << GAMEDATA_JSON;
			val = strchr( val + 1, ',' );
			tmp = strchr( val, '"' );
			while(val && val < tmp)
			{
				++val;
				data->category[ Config::AtoI( val ) ] |= 1 << GAMEDATA_JSON;
				val = strchr( val, ',' );
			}
		} else if(strncmp( tmp, "ver", 3 ) == 0)
		{
			// 設定のバージョン。
			data->ver[ GAMEDATA_JSON ] = Config::AtoI( val );
			msg.SetMessageFormat( "Version = %d", data->ver[ GAMEDATA_JSON ] );
		} else if(strncmp( tmp, "gver", 4 ) == 0)
		{
			// 設定のバージョン。
			data->gver[ GAMEDATA_JSON ] = Config::AtoI( val );
			msg.SetMessageFormat( "GameVersion = %d", data->gver[ GAMEDATA_JSON ] );
		}

	}

	msg.SetMessageFormat( "GameFile loaded." );

	return 0;
}

int GameDataCmp( const void *a, const void *b )
{
	return ( *( struct GameDataList ** )b )->number - ( *( struct GameDataList ** )a )->number;
}

int GameData::CreateGameTable( void )
{
	int i;
	struct GameDataList *tmp;
	// ゲームの配列を作成。
	MikanSystem->Lock( 4 );
	if(gametable)
	{
		free( gametable );
	}

	gametable = ( struct GameDataList ** )calloc( gamemax, sizeof( struct GameDataList * ) );
	tmp = gamelist;
	for(i = 0 ; i < gamemax ; ++i)
	{
		gametable[ i ] = tmp;
		tmp = tmp->next;
	}

	// ソート。
	qsort( gametable, gamemax, sizeof( struct GameDataList * ), GameDataCmp );

	if(0 < gamemax)
	{
		// リンク貼り直し。
		for(i = 0 ; i < gamemax - 1 ; ++i)
		{
			gametable[ i ]->next = gametable[ i + 1 ];
		}
		gametable[ gamemax - 1 ]->next = NULL;
		gamelist = gametable[ 0 ];
	}
	MikanSystem->Unlock( 4 );

	return 0;
}

int GameData::SetArea( int x, int y, int w, int h )
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

	mw = ( w - 70 ) / ( fontsize * 5 / 13 );
	mh = h / fontsize;
	format[ 0 ] = '%';
	format[ 1 ] = '.';
	sprintf_s( format + 2, 16 - 2, "%ds", mw );
	vgamenum = h / fontsize;
	return 0;
}

int GameData::Draw( int inputlock )
{
	int i, dx, dy, num;
	int sstart = 20, sbar = h - 40;
	int max, prog;
	int view = 0;
	int my, mx;
	int scroll;

	MikanSystem->Lock( 4 );
	if(gametable == NULL)
	{
		MikanDraw->Printf( 4, x, y, "Now Loading ..." );
		return 1;
	}

	for(i = 0 ; i < vgamenum && ( num = i + begin ) < gamemax ; ++i)
	{
		//num = i + begin;
		dy = y + i * fontsize;

		++view;
		if(num == target)
		{
			// プログレスバーの描画。
			max = this->max;
			prog = progress;
			if(max < 0)
			{
				max = 1;
			}
			if(max < prog)
			{
				prog = max;
			}
			MikanDraw->DrawTextureScaling( 0, x, dy, 690, 65 + mode * 5, 5, 5, w * prog / max, fontsize );
		} else if(inputlock && MouseOver( x, dy, w, fontsize ))
		{
			// 範囲内にマウスカーソルがある。
			MikanDraw->DrawTextureScaling( 0, x, dy, 690, 60, 5, 5, w, fontsize );
			if(MikanInput->GetMouseNum( 0 ) == -1)
			{
				select = num;
				if(MikanInput->GetMousePosX() < 25)
				{
					// チェックボックスの切り替え。
					gametable[ select ]->check = ( gametable[ select ]->check + 1 ) % 2;
				}
			}
		}

		// チェックボックス。
		MikanDraw->DrawTexture( 0, x, dy, 660, 40 + ( gametable[ num ]->check ) * 20, 20, 20 );
		if(gametable[ num ]->ver[ GAMEDATA_DIR ] < gametable[ num ]->ver[ GAMEDATA_JSON ])
		{
			// アップデートあり。
			MikanDraw->DrawTexture( 0, x + 20, dy, 640, 60, 20, 20 );
		}
		if(0 <= gametable[ num ]->ver[ GAMEDATA_DIR ])
		{
			// インストール済み。
			MikanDraw->DrawTexture( 0, x + 40, dy, 640, 40, 20, 20 );
		}
		// ゲームタイトル。
		MikanDraw->Printf( 4, x + 65, dy, format, gametable[ num ]->title[ GAMEDATA_DIR ] ? gametable[ num ]->title[ GAMEDATA_DIR ] : gametable[ num ]->title[ GAMEDATA_JSON ] );
	}

	scroll = DrawScroll( x + 340, y, 340, gamemax <= 1 ? 1 : ( gamemax - 1 ), view, begin );

	// 選択しているゲームの詳細情報。
	if(0 <= select)
	{
		dx = x + 350;
		dy = y;
		num = gametable[ select ]->title[ GAMEDATA_DIR ] ? GAMEDATA_DIR : GAMEDATA_JSON;

		MikanDraw->Printf( 0, dx, dy, "%s", gametable[ select ]->title[ num ] );
	}

	if(MouseOver( x, y, w, h ) || scroll)
	{
		if(MikanInput->GetMouseWheel() > 0)
		{
			if(0 < begin)
			{
				--begin;
			}
		} else if(MikanInput->GetMouseWheel() < 0)
		{
			if(begin + mh + 1 < gamemax)
			{
				++begin;
			}
		} else if(scroll)
		{
			if(scroll < 0)
			{
				if(0 < begin)
				{
					--begin;
				}
			} else
			{
				if(begin + mh + 1 < gamemax)
				{
					++begin;
				}
			}
		}
	}

	MikanSystem->Unlock( 4 );

	return 0;
}
